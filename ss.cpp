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

class HTTPClient {
private:
    int socket_fd;
    std::string host;
    int port;

public:
    HTTPClient(const std::string& hostname, int port_num) : host(hostname), port(port_num), socket_fd(-1) {}
    
    ~HTTPClient() {
        if (socket_fd != -1) {
            close(socket_fd);
        }
    }
    
    bool connect() {
        // Create socket
        socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_fd < 0) {
            std::cerr << "Error creating socket" << std::endl;
            return false;
        }
        
        // Get host by name
        struct hostent* server = gethostbyname(host.c_str());
        if (server == nullptr) {
            std::cerr << "Error: no such host " << host << std::endl;
            return false;
        }
        
        // Setup server address
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        memcpy(&server_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
        server_addr.sin_port = htons(port);
        
        // Connect to server
        if (::connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "Error connecting to server" << std::endl;
            return false;
        }
        
        return true;
    }
    
    bool sendTextPost(const std::string& path) {
        if (socket_fd == -1) {
            std::cerr << "Not connected to server" << std::endl;
            return false;
        }
        
        // Calculate approximate body size (numbers 1-100000 with newlines)
        size_t estimated_body_size = 0;
        for (int i = 1; i <= 100000; i++) {
            estimated_body_size += std::to_string(i).length() + 1; // +1 for \n
        }
        
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
        
        // Send headers first
        std::cout << "Sending headers..." << std::endl;
        ssize_t sent = send(socket_fd, header_str.c_str(), header_str.length(), 0);
        if (sent < 0) {
            std::cerr << "Error sending headers" << std::endl;
            return false;
        }
        std::cout << "Headers sent (" << sent << " bytes)" << std::endl;
        
        // Small delay to demonstrate separate sending
        usleep(500000); // 500ms delay - longer delay to see separation clearly
        
        // Send body data in chunks (streaming)
        std::cout << "Streaming text body data..." << std::endl;
        const int chunk_size = 5000; // Send 5000 numbers at a time (bigger chunks)
        
        for (int start = 1; start <= 100000; start += chunk_size) {
            int end = std::min(start + chunk_size - 1, 100000);
            
            // Build text chunk
            std::ostringstream chunk_stream;
            for (int i = start; i <= end; i++) {
                chunk_stream << i << "\n";
            }
            
            std::string chunk_data = chunk_stream.str();
            
            // Send chunk with proper error handling
            const char* data_ptr = chunk_data.c_str();
            size_t total_to_send = chunk_data.length();
            size_t total_sent = 0;
            
            while (total_sent < total_to_send) {
                ssize_t chunk_sent = send(socket_fd, data_ptr + total_sent, total_to_send - total_sent, 0);
                if (chunk_sent < 0) {
                    std::cerr << "Error sending chunk starting at " << start << std::endl;
                    return false;
                }
                total_sent += chunk_sent;
            }
            
            std::cout << "Sent chunk: " << start << " to " << end 
                     << " (" << total_sent << " bytes)" << std::endl;
            
            // Delay between chunks to demonstrate streaming
            usleep(1000000); // 1 second delay - very obvious separation
        }
        
        std::cout << "All data sent successfully!" << std::endl;
        return true;
    }
    
    std::string receiveResponse() {
        if (socket_fd == -1) {
            return "";
        }
        
        char buffer[4096];
        std::string response;
        
        while (true) {
            ssize_t bytes_received = recv(socket_fd, buffer, sizeof(buffer) - 1, 0);
            if (bytes_received <= 0) {
                break;
            }
            buffer[bytes_received] = '\0';
            response += buffer;
            
            // Check if we've received the complete response
            if (response.find("\r\n\r\n") != std::string::npos) {
                break;
            }
        }
        
        return response;
    }
};

int main() {
    // Configuration
    std::string hostname = "127.0.0.1"; // localhost
    int port = 8080;
    std::string path = "/";
    
    std::cout << "Starting HTTP POST with text streaming..." << std::endl;
    std::cout << "Target: " << hostname << ":" << port << path << std::endl;
    
    // Create client and connect
    HTTPClient client(hostname, port);
    
    if (!client.connect()) {
        std::cerr << "Failed to connect to server" << std::endl;
        return 1;
    }
    
    std::cout << "Connected to server" << std::endl;
    
    // Send POST request with text streaming
    if (!client.sendTextPost(path)) {
        std::cerr << "Failed to send POST request" << std::endl;
        return 1;
    }
    
    // Receive and display response
    std::cout << "\nReceiving response..." << std::endl;
    std::string response = client.receiveResponse();
    std::cout << "Response received:" << std::endl;
    std::cout << response << std::endl;
    
    return 0;
}

// Compilation instructions:
// g++ -o http_client test.cpp
// 
// Usage:
// ./http_client
//
// Note: This example connects to localhost:8080
// Make sure you have a server running on that address.
// The data will be sent as text format: "1\n2\n3\n...100000\n"
