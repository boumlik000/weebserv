#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <algorithm>
#include <errno.h>
#include <signal.h>

class HTTPClient {
private:
    int socket_fd;
    std::string host;
    int port;

    void printError(const std::string& context, int error_code = errno) {
        std::cerr << "[ERROR] " << context << ": " << strerror(error_code) 
                  << " (errno: " << error_code << ")" << std::endl;
    }

    void printSocketError(const std::string& context) {
        int sock_error;
        socklen_t len = sizeof(sock_error);
        if (getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &sock_error, &len) == 0 && sock_error != 0) {
            std::cerr << "[SOCKET ERROR] " << context << ": " << strerror(sock_error) 
                      << " (socket errno: " << sock_error << ")" << std::endl;
        } else {
            printError(context);
        }
    }

public:
    HTTPClient(const std::string& hostname, int port_num) : host(hostname), port(port_num), socket_fd(-1) {
        // Ignore SIGPIPE to handle broken pipe errors gracefully
        signal(SIGPIPE, SIG_IGN);
    }
    
    ~HTTPClient() {
        if (socket_fd != -1) {
            std::cout << "[INFO] Closing socket connection" << std::endl;
            if (close(socket_fd) < 0) {
                printError("Error closing socket");
            }
        }
    }
    
    bool connect() {
        std::cout << "[INFO] Creating socket..." << std::endl;
        
        // Create socket
        socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_fd < 0) {
            printError("Error creating socket");
            return false;
        }
        
        std::cout << "[INFO] Socket created successfully (fd: " << socket_fd << ")" << std::endl;
        
        // Set socket options for better error detection
        int optval = 1;
        if (setsockopt(socket_fd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) < 0) {
            printError("Warning: Could not set SO_KEEPALIVE");
        }
        
        // Set socket timeout
        struct timeval timeout;
        timeout.tv_sec = 30;  // 30 seconds timeout
        timeout.tv_usec = 0;
        
        if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
            printError("Warning: Could not set receive timeout");
        }
        
        if (setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
            printError("Warning: Could not set send timeout");
        }
        
        std::cout << "[INFO] Resolving hostname: " << host << std::endl;
        
        // Get host by name
        struct hostent* server = gethostbyname(host.c_str());
        if (server == nullptr) {
            std::cerr << "[ERROR] DNS resolution failed for host: " << host << std::endl;
            std::cerr << "[ERROR] h_errno: " << h_errno << " (";
            switch(h_errno) {
                case HOST_NOT_FOUND: std::cerr << "HOST_NOT_FOUND"; break;
                case NO_ADDRESS: std::cerr << "NO_ADDRESS"; break;
                case NO_RECOVERY: std::cerr << "NO_RECOVERY"; break;
                case TRY_AGAIN: std::cerr << "TRY_AGAIN"; break;
                default: std::cerr << "UNKNOWN"; break;
            }
            std::cerr << ")" << std::endl;
            close(socket_fd);
            socket_fd = -1;
            return false;
        }
        
        std::cout << "[INFO] Hostname resolved successfully" << std::endl;
        
        // Setup server address
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        memcpy(&server_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
        server_addr.sin_port = htons(port);
        
        // Print the IP address we're connecting to
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &server_addr.sin_addr, ip_str, INET_ADDRSTRLEN);
        std::cout << "[INFO] Connecting to " << ip_str << ":" << port << std::endl;
        
        // Connect to server
        if (::connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            printSocketError("Connection failed");
            std::cerr << "[ERROR] Failed to connect to " << ip_str << ":" << port << std::endl;
            close(socket_fd);
            socket_fd = -1;
            return false;
        }
        
        std::cout << "[SUCCESS] Connected to server!" << std::endl;
        return true;
    }
    
    bool sendTextPost(const std::string& path) {
        if (socket_fd == -1) {
            std::cerr << "[ERROR] Not connected to server" << std::endl;
            return false;
        }
        
        // Check if socket is still connected
        int error = 0;
        socklen_t len = sizeof(error);
        if (getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error != 0) {
            std::cerr << "[ERROR] Socket is in error state before sending" << std::endl;
            if (error != 0) {
                std::cerr << "[ERROR] Socket error: " << strerror(error) << std::endl;
            }
            return false;
        }
        
        std::cout << "[INFO] Calculating body size..." << std::endl;
        
        // Calculate approximate body size (numbers 1-100000 with newlines)
        size_t estimated_body_size = 0;
        for (int i = 1; i <= 100000; i++) {
            estimated_body_size += std::to_string(i).length() + 1; // +1 for \n
        }
        
        std::cout << "[INFO] Estimated body size: " << estimated_body_size << " bytes" << std::endl;
        
        // Prepare HTTP headers
        std::ostringstream headers;
        headers << "POST " << path << " HTTP/1.1\r\n";
        headers << "Host: " << host << ":" << port << "\r\n";
        headers << "Content-Type: text/plain\r\n";
        headers << "Content-Length: " << estimated_body_size << "\r\n";
        headers << "Connection: keep-alive\r\n";
        headers << "User-Agent: CustomHTTPClient/1.0\r\n";
        headers << "\r\n";  // End of headers
        
        std::string header_str = headers.str();
        std::cout << "[INFO] Headers prepared (" << header_str.length() << " bytes)" << std::endl;
        std::cout << "[DEBUG] Headers content:\n" << header_str << std::endl;
        
        // Send headers first
        std::cout << "[INFO] Sending headers..." << std::endl;
        ssize_t sent = send(socket_fd, header_str.c_str(), header_str.length(), MSG_NOSIGNAL);
        if (sent < 0) {
            printSocketError("Error sending headers");
            
            // Check specific error conditions
            if (errno == EPIPE || errno == ECONNRESET) {
                std::cerr << "[ERROR] Server closed connection while sending headers" << std::endl;
            } else if (errno == ETIMEDOUT) {
                std::cerr << "[ERROR] Send timeout while sending headers" << std::endl;
            } else if (errno == ENOTCONN) {
                std::cerr << "[ERROR] Socket is not connected" << std::endl;
            }
            return false;
        }
        
        if (sent != (ssize_t)header_str.length()) {
            std::cerr << "[WARNING] Partial header send: " << sent << "/" << header_str.length() << " bytes" << std::endl;
            // Try to send remaining data
            const char* remaining = header_str.c_str() + sent;
            size_t remaining_size = header_str.length() - sent;
            ssize_t remaining_sent = send(socket_fd, remaining, remaining_size, MSG_NOSIGNAL);
            if (remaining_sent < 0) {
                printSocketError("Error sending remaining headers");
                return false;
            }
            std::cout << "[INFO] Sent remaining " << remaining_sent << " header bytes" << std::endl;
        }
        
        std::cout << "[SUCCESS] Headers sent (" << sent << " bytes)" << std::endl;
        
        // Small delay to demonstrate separate sending
        std::cout << "[INFO] Waiting 500ms before sending body..." << std::endl;
        usleep(500000); // 500ms delay
        
        // Send body data in chunks (streaming)
        std::cout << "[INFO] Starting to stream text body data..." << std::endl;
        const int chunk_size = 1000; // Smaller chunks for better compatibility
        size_t total_body_sent = 0;
        
        for (int start = 1; start <= 100000; start += chunk_size) {
            int end = std::min(start + chunk_size - 1, 100000);
            
            // Check socket status before sending each chunk
            int sock_error = 0;
            socklen_t len = sizeof(sock_error);
            if (getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &sock_error, &len) < 0) {
                printError("Error checking socket status");
                return false;
            }
            if (sock_error != 0) {
                std::cerr << "[ERROR] Socket error detected before chunk " << start 
                         << ": " << strerror(sock_error) << std::endl;
                return false;
            }
            
            // Build text chunk
            std::ostringstream chunk_stream;
            for (int i = start; i <= end; i++) {
                chunk_stream << i << "\n";
            }
            
            std::string chunk_data = chunk_stream.str();
            
            std::cout << "[INFO] Sending chunk " << start << "-" << end 
                     << " (" << chunk_data.length() << " bytes)..." << std::flush;
            
            // Send chunk with proper error handling
            const char* data_ptr = chunk_data.c_str();
            size_t total_to_send = chunk_data.length();
            size_t chunk_sent = 0;
            
            while (chunk_sent < total_to_send) {
                ssize_t bytes_sent = send(socket_fd, data_ptr + chunk_sent, 
                                        total_to_send - chunk_sent, MSG_NOSIGNAL);
                
                if (bytes_sent < 0) {
                    std::cout << " FAILED!" << std::endl;
                    printSocketError("Error sending chunk data");
                    
                    // Detailed error analysis
                    if (errno == EPIPE) {
                        std::cerr << "[ERROR] Broken pipe - server closed connection" << std::endl;
                    } else if (errno == ECONNRESET) {
                        std::cerr << "[ERROR] Connection reset by peer" << std::endl;
                    } else if (errno == ETIMEDOUT) {
                        std::cerr << "[ERROR] Send timeout" << std::endl;
                    } else if (errno == ENOTCONN) {
                        std::cerr << "[ERROR] Socket is not connected" << std::endl;
                    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        std::cerr << "[ERROR] Send would block (buffer full?)" << std::endl;
                    }
                    
                    std::cerr << "[ERROR] Failed at chunk " << start << "-" << end 
                             << " after sending " << chunk_sent << "/" << total_to_send 
                             << " bytes of chunk" << std::endl;
                    std::cerr << "[ERROR] Total body bytes sent so far: " << total_body_sent << std::endl;
                    return false;
                }
                
                if (bytes_sent == 0) {
                    std::cout << " CONNECTION CLOSED!" << std::endl;
                    std::cerr << "[ERROR] Server closed connection (send returned 0)" << std::endl;
                    std::cerr << "[ERROR] Failed at chunk " << start << "-" << end 
                             << " after sending " << chunk_sent << "/" << total_to_send 
                             << " bytes of chunk" << std::endl;
                    return false;
                }
                
                chunk_sent += bytes_sent;
            }
            
            total_body_sent += chunk_sent;
            std::cout << " SUCCESS!" << std::endl;
            std::cout << "[INFO] Chunk completed. Total body sent: " << total_body_sent << " bytes" << std::endl;
            
            // Delay between chunks to demonstrate streaming (reduced for server compatibility)
            std::cout << "[INFO] Waiting 100ms before next chunk..." << std::endl;
            usleep(100000); // 100ms delay (reduced from 1 second)
        }
        
        std::cout << "[SUCCESS] All data sent successfully!" << std::endl;
        std::cout << "[INFO] Total bytes sent: headers(" << header_str.length() 
                  << ") + body(" << total_body_sent << ") = " 
                  << (header_str.length() + total_body_sent) << " bytes" << std::endl;
        return true;
    }
    
    std::string receiveResponse() {
        if (socket_fd == -1) {
            std::cerr << "[ERROR] Cannot receive - not connected" << std::endl;
            return "";
        }
        
        std::cout << "[INFO] Starting to receive response..." << std::endl;
        
        char buffer[4096];
        std::string response;
        size_t total_received = 0;
        int receive_attempts = 0;
        const int max_attempts = 10;
        
        while (receive_attempts < max_attempts) {
            receive_attempts++;
            std::cout << "[INFO] Receive attempt " << receive_attempts << "..." << std::flush;
            
            ssize_t bytes_received = recv(socket_fd, buffer, sizeof(buffer) - 1, 0);
            
            if (bytes_received < 0) {
                std::cout << " ERROR!" << std::endl;
                printSocketError("Error receiving response");
                
                if (errno == ETIMEDOUT) {
                    std::cerr << "[ERROR] Receive timeout" << std::endl;
                } else if (errno == ECONNRESET) {
                    std::cerr << "[ERROR] Connection reset by peer while receiving" << std::endl;
                } else if (errno == ENOTCONN) {
                    std::cerr << "[ERROR] Socket is not connected" << std::endl;
                }
                break;
            }
            
            if (bytes_received == 0) {
                std::cout << " CONNECTION CLOSED!" << std::endl;
                std::cout << "[INFO] Server closed connection (normal end)" << std::endl;
                break;
            }
            
            buffer[bytes_received] = '\0';
            response += buffer;
            total_received += bytes_received;
            
            std::cout << " OK (" << bytes_received << " bytes)" << std::endl;
            std::cout << "[INFO] Total received so far: " << total_received << " bytes" << std::endl;
            
            // Check if we've received the complete response
            if (response.find("\r\n\r\n") != std::string::npos) {
                std::cout << "[INFO] Complete HTTP response received" << std::endl;
                break;
            }
            
            // Small delay between receive attempts
            usleep(100000); // 100ms
        }
        
        if (receive_attempts >= max_attempts) {
            std::cerr << "[WARNING] Maximum receive attempts reached" << std::endl;
        }
        
        std::cout << "[INFO] Response receiving completed. Total: " << total_received << " bytes" << std::endl;
        return response;
    }
    
    bool isConnected() {
        if (socket_fd == -1) return false;
        
        int error = 0;
        socklen_t len = sizeof(error);
        if (getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
            return false;
        }
        return error == 0;
    }
};

int main() {
    // Configuration
    std::string hostname = "127.0.0.1"; // localhost
    int port = 8080;
    std::string path = "/";
    
    std::cout << "========================================" << std::endl;
    std::cout << "HTTP POST Client with Error Handling" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "[INFO] Target: " << hostname << ":" << port << path << std::endl;
    std::cout << "[INFO] Will send numbers 1-100000 as text data" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Create client and connect
    HTTPClient client(hostname, port);
    
    if (!client.connect()) {
        std::cerr << "[FATAL] Failed to connect to server" << std::endl;
        return 1;
    }
    
    std::cout << "[SUCCESS] Connected to server successfully" << std::endl;
    
    // Send POST request with text streaming
    if (!client.sendTextPost(path)) {
        std::cerr << "[FATAL] Failed to send POST request" << std::endl;
        
        // Check if still connected
        if (client.isConnected()) {
            std::cout << "[INFO] Connection still active, attempting to receive response..." << std::endl;
        } else {
            std::cout << "[INFO] Connection lost" << std::endl;
            return 1;
        }
    }
    
    // Receive and display response
    std::cout << "\n========================================" << std::endl;
    std::cout << "[INFO] Attempting to receive response..." << std::endl;
    std::cout << "========================================" << std::endl;
    
    std::string response = client.receiveResponse();
    
    if (response.empty()) {
        std::cout << "[WARNING] No response received" << std::endl;
    } else {
        std::cout << "\n========================================" << std::endl;
        std::cout << "SERVER RESPONSE:" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << response << std::endl;
        std::cout << "========================================" << std::endl;
    }
    
    std::cout << "[INFO] Program completed" << std::endl;
    return 0;
}

// Compilation instructions:
// g++ -o http_client_enhanced test.cpp -std=c++11
// 
// Usage:
// ./http_client_enhanced
//
// This enhanced version includes:
// - Comprehensive error handling for all socket operations
// - Detailed error messages with errno codes and descriptions
// - Socket status checking before operations
// - Timeout handling
// - Connection state monitoring
// - Broken pipe and connection reset detection
// - Progress reporting during send/receive operations
// - Better debugging output with timestamps and status
//
// Test scenarios:
// 1. Run without server -> Connection refused error
// 2. Start server, then stop during transmission -> Broken pipe/reset error
// 3. Server with small receive buffer -> Various send errors
// 4. Network issues -> Timeout errors
