#include "wb0.hpp"

bool EpollServer::setupServer() 
{   
    // Step 1: Create server socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "❌ Socket creation failed!" << std::endl;
        return false;
    }
    std::cout << "✅ Socket created: " << server_fd << std::endl;
    
    // Step 2: Set socket options
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "❌ Setsockopt failed!" << std::endl;
        return false;
    }
    std::cout << "✅ Socket options set" << std::endl;
    
    // Step 3: Make server socket non-blocking
    if (!setNonBlocking(server_fd)) {
        std::cerr << "❌ Failed to set non-blocking!" << std::endl;
        return false;
    }
    std::cout << "✅ Server socket set to non-blocking" << std::endl;
    
    // Step 4: Setup address
    // kanhowlo l port , l ip , w lipv4 l soccaddre
    // htons kathowl lport  l unsigned int ` w inet add katconverti l ip l soket
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1");  // 0.0.0.0
    address.sin_port = htons(1024);
    
    // Step 5: Bind
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "❌ Bind failed: " << strerror(errno) << std::endl;
        return false;
    }
    std::cout << "✅ Socket bound to port " << PORT << std::endl;
    
    // Step 6: Listen
    if (listen(server_fd, 128) < 0) {
        std::cerr << "❌ Listen failed!" << std::endl;
        return false;
    }
    std::cout << "✅ Server listening" << std::endl;
    
    // Step 7: Create epoll instance
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        std::cerr << "❌ Epoll create failed!" << std::endl;
        return false;
    }
    std::cout << "✅ Epoll instance created: " << epoll_fd << std::endl;
    
    // Step 8: Add server socket to epoll
    struct epoll_event event;
    event.events = EPOLLIN;  // Monitor for input
    event.data.fd = server_fd;
    
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
        std::cerr << "❌ Failed to add server to epoll!" << std::endl;
        return false;
    }
    std::cout << "✅ Server socket added to epoll monitoring" << std::endl;
    
    return true;
}

void EpollServer::handleNewClient() 
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    // Accept new connection
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd == -1) {
        if (errno != EWOULDBLOCK && errno != EAGAIN) {
            std::cerr << "❌ Accept error: " << strerror(errno) << std::endl;
        }
        return;
    }
    
    // Make client socket non-blocking
    if (!setNonBlocking(client_fd)) {
        std::cerr << "❌ Failed to set client non-blocking!" << std::endl;
        close(client_fd);
        return;
    }
    
    // Add client to epoll monitoring
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;  // Edge-triggered input
    event.data.fd = client_fd;
    
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
        std::cerr << "❌ Failed to add client to epoll!" << std::endl;
        close(client_fd);
        return;
    }
    
    // Store client info
    client_fds.push_back(client_fd);
    
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client_addr.sin_port);
    
    std::cout << "🎉 New client connected: " << client_ip << ":" << client_port 
            << " (fd: " << client_fd << ")" << std::endl;
    std::cout << "👥 Total clients: " << client_fds.size() << std::endl;
}


 void EpollServer::handleClientData(int client_fd) 
{
    char buffer[BUFFER_SIZE];

    while (true) 
    {  // Read all available data
        ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
    
        if (bytes_read > 0)
        {
            buffer[bytes_read] = '\0';
            std::cout << "📨 Received from client " << client_fd 
                        << " (" << bytes_read << " bytes):" << std::endl;
            std::cout << buffer << std::endl;
            
            // Send response
            const char* response = 
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: 100\r\n"
                "\r\n"
                "Hello from Epoll Server! NIHAHAHA 🚀";
            
            write(client_fd, response, strlen(response));
            std::cout << "📤 Response sent to client " << client_fd << std::endl;
            
        } else if (bytes_read == 0) {
            // Client disconnected
            std::cout << "👋 Client " << client_fd << " disconnected" << std::endl;
            removeClient(client_fd);
            break;
            
        } else {
                // Error or no more data
                if (errno != EWOULDBLOCK && errno != EAGAIN) {
                    std::cout << "❌ Read error from client " << client_fd 
                            << ": " << strerror(errno) << std::endl;
                    removeClient(client_fd);
                }
                break;  // No more data available
        }
    }
}


void EpollServer::removeClient(int client_fd) 
{
    // Remove from epoll
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
    
    // Close socket
    close(client_fd);
    
    // Remove from vector
    for (std::vector<int>::iterator it = client_fds.begin(); it != client_fds.end(); ++it) {
        if (*it == client_fd) {
            client_fds.erase(it);
            break;
        }
    }
    
    std::cout << "🧹 Client " << client_fd << " cleaned up" << std::endl;
    std::cout << "👥 Total clients: " << client_fds.size() << std::endl;
}

