#include"server.hpp"

Server::Server(): config(g_default_config){
}
Server::~Server(){
    for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it) {
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, it->first, NULL);
        close(it->first);
    }
    for (size_t i = 0; i < listening_fds.size(); ++i) {
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, listening_fds[i], NULL);
        close(listening_fds[i]);
    }
    if (epoll_fd != -1) {
        close(epoll_fd);
    }
}
Server::Server(const ConfigFile& _config) : config(_config){
    run();
}

Server& Server::operator=(const Server& rhs){
    (void)rhs;
    return *this;
}
Server::Server(const Server& src):config(src.config), epoll_fd(src.epoll_fd){
    listening_fds = src.listening_fds;
    clients = src.clients;
}

void Server::setupServer(){
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        std::cerr<<"creation epoll failed"<<std::endl;
        return ;
    }
    const std::vector<ListenInfo>& listenInfos = config.getListenInfos();

    for (size_t i = 0; i < listenInfos.size(); ++i) {
        int server_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (server_fd == -1) {
            perror("socket");
            continue;
        }
    
        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
        struct sockaddr_in address = listenInfos[i].addr;
        if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
            perror("bind");
            close(server_fd);
            continue;
        }
        
        if (listen(server_fd, 128) < 0) {
            perror("listen");
            close(server_fd);
            continue;
        }
        
        // fcntl(server_fd, F_SETFL, O_NONBLOCK);
    
        std::cout << "Server listening on " << listenInfos[i].ip << ":" << listenInfos[i].port << std::endl;
    
        listening_fds.push_back(server_fd);
    
        struct epoll_event event;
        event.events = EPOLLIN;
        event.data.fd = server_fd;
    
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
            perror("epoll_ctl ADD");
            close(server_fd);
            continue;
        }
        std::cout << "Socket " << server_fd << " added to epoll." << std::endl;
    }
}

void    Server::removeClient(int client_fd){
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
    close(client_fd);
    clients.erase(client_fd);
}
// //boumlik
// void Server::modifyEpollEvent(int fd, uint32_t new_events) {
//     struct epoll_event event;
//     event.data.fd = fd;
//     event.events = new_events;
//     if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event) == -1) {
//         perror("epoll_ctl: mod");
//     }
// }

void    Server::handleNewConnection(int listener_fd){
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept4(listener_fd, (struct sockaddr *)&client_addr, &client_len, SOCK_NONBLOCK);
        if (client_fd == -1) {
           if (errno != EAGAIN && errno != EWOULDBLOCK) {
                std::cerr << "Failed to accept connection: " << strerror(errno) << std::endl;
            }
            return;
        }
        // fcntl(client_fd, F_SETFL, O_NONBLOCK);
        struct epoll_event event;
        event.events = EPOLLIN | EPOLLOUT | EPOLLET;;
        event.data.fd = client_fd;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
            perror("epoll_ctl: add client_fd");
            close(client_fd);
            return;
        }
        // clients[client_fd] = Client(client_fd, config);
        clients.insert(std::make_pair(client_fd, Client(client_fd, config)));

        
        std::cout << "New connection established with fd: " << client_fd << std::endl;
}
// ف server.cpp
// void    Server::handleClientEvent(int client_fd){
//     Client& client = clients[client_fd];
//     try {
        
//         client.readRequest(); // خلي الكليان يتكلف بالقراءة
//         client.process();     // خليه يعالج الطلب ويصاوب الجواب
//         client.sendResponse();// خليه يصيفط الجواب
//     } catch (const std::exception& e) {
//         std::cerr << "Error handling client " << client_fd << ": " << e.what() << std::endl;
//         removeClient(client_fd); // إلى وقع شي خطأ، كنمسحوه
//         return;
//     }
//     if (client.isDone()) {
//         removeClient(client_fd);
//     }

// }
void Server::handleClientEvent(int client_fd, uint32_t events) {
    Client& client = clients[client_fd];
    if (events & (EPOLLHUP | EPOLLERR)) {
            // Connection closed or error
            std::cout << "Client disconnected (fd: " << client_fd << ")" << std::endl;
            removeClient(client_fd);
            return;
    }
    if (events & EPOLLIN) {
        if (client.getState() == AWAITING_REQUEST) {
            client.readRequest();
        }
        if (client.getState() == REQUEST_RECEIVED) {
            client.process();
            // دابا الجواب واجد ف _fullResponse و الحالة هي SENDING_RESPONSE
        }
    }
    if (events & EPOLLOUT) {
        if (client.getState() == SENDING_RESPONSE) {
            client.sendResponse();
        }
    }
    if (client.isDone()) {
        removeClient(client_fd);
        return; // صافي سالينا مع هاد الكليان
    }
}


void    Server::eventLoop(){
    struct epoll_event events_received[MAX_EVENTS];
    while (true)
    {
        int num_events = epoll_wait(epoll_fd, events_received, MAX_EVENTS, -1);
        for(int i = 0; i < num_events; i++)
        {
            int active_fd = events_received[i].data.fd;
            std::vector<int>::iterator it = std::find(listening_fds.begin(), listening_fds.end(), active_fd);
            if (it != listening_fds.end()) {
                handleNewConnection(active_fd);
            } else {
                handleClientEvent(active_fd, events_received[i].events);
            }
        }
    }
    
}
    void    Server::run(){
    setupServer();
    eventLoop();
}