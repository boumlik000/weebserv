#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

class WebServ {
private:
    int server_fd;
    int port;
    struct sockaddr_in address;
    
    std::string getUploadPageHTML() {
        return "<!DOCTYPE html>\n"
               "<html>\n"
               "<head>\n"
               "    <title>File Upload</title>\n"
               "    <style>\n"
               "        body { font-family: Arial, sans-serif; margin: 50px; }\n"
               "        .upload-container { max-width: 500px; margin: 0 auto; }\n"
               "        .upload-area {\n"
               "            border: 2px dashed #ccc;\n"
               "            border-radius: 10px;\n"
               "            width: 100%;\n"
               "            height: 200px;\n"
               "            text-align: center;\n"
               "            padding: 20px;\n"
               "            margin: 20px 0;\n"
               "        }\n"
               "        .upload-btn {\n"
               "            background-color: #4CAF50;\n"
               "            color: white;\n"
               "            padding: 15px 30px;\n"
               "            border: none;\n"
               "            border-radius: 4px;\n"
               "            cursor: pointer;\n"
               "            font-size: 16px;\n"
               "        }\n"
               "        .upload-btn:hover { background-color: #45a049; }\n"
               "        input[type=file] { margin: 20px 0; }\n"
               "    </style>\n"
               "</head>\n"
               "<body>\n"
               "    <div class='upload-container'>\n"
               "        <h1>File Upload</h1>\n"
               "        <form id='uploadForm' action='/upload' method='POST' enctype='application/octet-stream'>\n"
               "            <div class='upload-area'>\n"
               "                <p>Choose a file to upload</p>\n"
               "                <input type='file' id='fileInput' name='file' required>\n"
               "            </div>\n"
               "            <button type='submit' class='upload-btn'>Upload File</button>\n"
               "        </form>\n"
               "    </div>\n"
               "    <script>\n"
               "        document.getElementById('uploadForm').onsubmit = function(e) {\n"
               "            e.preventDefault();\n"
               "            var fileInput = document.getElementById('fileInput');\n"
               "            var file = fileInput.files[0];\n"
               "            if (file) {\n"
               "                var xhr = new XMLHttpRequest();\n"
               "                xhr.open('POST', '/upload', true);\n"
               "                xhr.setRequestHeader('Content-Type', 'application/octet-stream');\n"
               "                xhr.setRequestHeader('X-File-Name', file.name);\n"
               "                xhr.onreadystatechange = function() {\n"
               "                    if (xhr.readyState === 4) {\n"
               "                        if (xhr.status === 200) {\n"
               "                            alert('File uploaded successfully!');\n"
               "                        } else {\n"
               "                            alert('Upload failed!');\n"
               "                        }\n"
               "                    }\n"
               "                };\n"
               "                xhr.send(file);\n"
               "            }\n"
               "        };\n"
               "    </script>\n"
               "</body>\n"
               "</html>";
    }
    
    std::string getHTTPResponse(const std::string& body, const std::string& contentType = "text/html") {
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: " << contentType << "\r\n";
        response << "Content-Length: " << body.length() << "\r\n";
        response << "Connection: close\r\n";
        response << "\r\n";
        response << body;
        return response.str();
    }
    
    std::string parseRequestLine(const std::string& request) {
        size_t end = request.find("\r\n");
        if (end == std::string::npos) {
            end = request.find("\n");
        }
        return request.substr(0, end);
    }
    
    std::string getRequestPath(const std::string& requestLine) {
        size_t start = requestLine.find(" ");
        if (start == std::string::npos) return "/";
        start++;
        
        size_t end = requestLine.find(" ", start);
        if (end == std::string::npos) return "/";
        
        return requestLine.substr(start, end - start);
    }
    
    std::string getRequestMethod(const std::string& requestLine) {
        size_t end = requestLine.find(" ");
        if (end == std::string::npos) return "GET";
        return requestLine.substr(0, end);
    }
    
    std::string getRequestBody(const std::string& request) {
        size_t headerEnd = request.find("\r\n\r\n");
        if (headerEnd == std::string::npos) {
            headerEnd = request.find("\n\n");
            if (headerEnd == std::string::npos) return "";
            headerEnd += 2;
        } else {
            headerEnd += 4;
        }
        
        if (headerEnd >= request.length()) return "";
        return request.substr(headerEnd);
    }
    
    bool isContentTypeOctetStream(const std::string& request) {
        size_t pos = request.find("Content-Type:");
        if (pos == std::string::npos) return false;
        
        size_t lineEnd = request.find("\r\n", pos);
        if (lineEnd == std::string::npos) {
            lineEnd = request.find("\n", pos);
            if (lineEnd == std::string::npos) return false;
        }
        
        std::string contentTypeLine = request.substr(pos, lineEnd - pos);
        return contentTypeLine.find("application/octet-stream") != std::string::npos;
    }
    
    std::string getFileName(const std::string& request) {
        size_t pos = request.find("X-File-Name:");
        if (pos == std::string::npos) return "unknown_file";
        
        pos += 12; // Skip "X-File-Name:"
        while (pos < request.length() && (request[pos] == ' ' || request[pos] == '\t')) {
            pos++;
        }
        
        size_t lineEnd = request.find("\r\n", pos);
        if (lineEnd == std::string::npos) {
            lineEnd = request.find("\n", pos);
            if (lineEnd == std::string::npos) lineEnd = request.length();
        }
        
        return request.substr(pos, lineEnd - pos);
    }
    
    void printRawRequest(const std::string& request) {
        std::cout << "\n=== RAW REQUEST AS SENT BY CLIENT ===" << std::endl;
        std::cout << request;
        std::cout << "\n=== END OF RAW REQUEST ===" << std::endl << std::endl;
    }
    
public:
    WebServ(int p) : port(p), server_fd(-1) {}
    
    ~WebServ() {
        if (server_fd != -1) {
            close(server_fd);
        }
    }
    
    bool start() {
        // Create socket
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd == -1) {
            std::cerr << "Failed to create socket" << std::endl;
            return false;
        }
        
        // Set socket options
        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            std::cerr << "Failed to set socket options" << std::endl;
            return false;
        }
        
        // Setup address structure
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);
        
        // Bind socket
        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            std::cerr << "Failed to bind socket to port " << port << std::endl;
            return false;
        }
        
        // Listen for connections
        if (listen(server_fd, 10) < 0) {
            std::cerr << "Failed to listen on socket" << std::endl;
            return false;
        }
        
        std::cout << "WebServ started on port " << port << std::endl;
        std::cout << "Visit http://localhost:" << port << " to access the upload page" << std::endl;
        return true;
    }
    
    void run() {
        while (true) {
            socklen_t addrlen = sizeof(address);
            int client_fd = accept(server_fd, (struct sockaddr*)&address, &addrlen);
            
            if (client_fd < 0) {
                std::cerr << "Failed to accept client connection" << std::endl;
                continue;
            }
            
            // Read request
            char buffer[8192] = {0};
            ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
            
            if (bytes_read <= 0) {
                close(client_fd);
                continue;
            }
            
            std::string request(buffer, bytes_read);
            
            // Read more data if Content-Length indicates more data
            size_t contentLengthPos = request.find("Content-Length:");
            if (contentLengthPos != std::string::npos) {
                size_t lineEnd = request.find("\r\n", contentLengthPos);
                if (lineEnd == std::string::npos) lineEnd = request.find("\n", contentLengthPos);
                
                if (lineEnd != std::string::npos) {
                    std::string lengthLine = request.substr(contentLengthPos + 15, lineEnd - contentLengthPos - 15);
                    int contentLength = atoi(lengthLine.c_str());
                    
                    std::string body = getRequestBody(request);
                    while ((int)body.length() < contentLength && bytes_read > 0) {
                        bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
                        if (bytes_read > 0) {
                            request.append(buffer, bytes_read);
                            body = getRequestBody(request);
                        }
                    }
                }
            }
            
            // Parse request
            std::string requestLine = parseRequestLine(request);
            std::string method = getRequestMethod(requestLine);
            std::string path = getRequestPath(requestLine);
            std::string body = getRequestBody(request);
            
            std::cout << "Request: " << method << " " << path << std::endl;
            
            std::string response;
            
            if (path == "/" && method == "GET") {
                // Serve upload page
                std::string htmlContent = getUploadPageHTML();
                response = getHTTPResponse(htmlContent);
            }
            else if (path == "/upload" && method == "POST") {
                if (isContentTypeOctetStream(request)) {
                    // Print the raw request exactly as sent by client
                    printRawRequest(request);
                    
                    response = getHTTPResponse("File uploaded successfully!", "text/plain");
                } else {
                    response = getHTTPResponse("Invalid content type. Expected application/octet-stream", "text/plain");
                }
            }
            else {
                // 404 Not Found
                std::string notFoundBody = "404 Not Found";
                std::ostringstream notFoundResponse;
                notFoundResponse << "HTTP/1.1 404 Not Found\r\n";
                notFoundResponse << "Content-Type: text/plain\r\n";
                notFoundResponse << "Content-Length: " << notFoundBody.length() << "\r\n";
                notFoundResponse << "Connection: close\r\n";
                notFoundResponse << "\r\n";
                notFoundResponse << notFoundBody;
                response = notFoundResponse.str();
            }
            
            // Send response
            send(client_fd, response.c_str(), response.length(), 0);
            close(client_fd);
        }
    }
};

int main(int argc, char* argv[]) {
    int port = 8080;
    
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            std::cerr << "Invalid port number. Using default port 8080" << std::endl;
            port = 8080;
        }
    }
    
    WebServ server(port);
    
    if (!server.start()) {
        return 1;
    }
    
    server.run();
    return 0;
}