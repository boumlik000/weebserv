#include"server.hpp"

Server::Server(): config(g_default_config){
}
Server::~Server(){
    for (std::map<int, Client*>::iterator it = clients.begin(); it != clients.end(); ++it) {
        delete it->second; // كمسحو الأوبجيكت
        close(it->first);
    }
    clients.clear(); // كنخويو الـ map
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
    epoll_fd = epoll_create1(EPOLL_CLOEXEC);
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
    std::cerr<<"disconnected "<<client_fd<<std::endl;
    // فـ server.cpp -> removeClient
    std::map<int, Client*>::iterator it = clients.find(client_fd);
    if (it != clients.end()) {
        delete it->second; // <== كنزيدو هادي باش نمسحو الأوبجيكت
        clients.erase(it); // كنمسحو المؤشر من الـ map
    }
    notifEvent.erase(client_fd);
}
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
    // فـ server.cpp -> handleNewConnection()
// فـ server.cpp -> handleNewConnection
    Client* new_client = new Client(client_fd, config);
    clients.insert(std::make_pair(client_fd, new_client));    // clients.insert(std::make_pair(client_fd, Client(client_fd, config)));
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLOUT | EPOLLET;;
    event.data.fd = client_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
        perror("epoll_ctl: add client_fd");
        close(client_fd);
        return;
    }

    
    std::cout << "New connection established with fd: " << client_fd << std::endl;
}
// فـ server.cpp
void Server::checkTimeouts() {
    time_t now = time(NULL);
    std::vector<int> clients_to_remove;

    for (std::map<int, Client *>::iterator it = clients.begin(); it != clients.end(); ++it) {
        double idle_seconds = difftime(now, it->second->getLastActivity());

        if (idle_seconds > TIMEOUT_SECONDS) {
            std::cout << "\033[36mClient " << it->first << " has timed out due to inactivity.\033[0m" << std::endl;
            clients_to_remove.push_back(it->first);
        }
    }

    for (size_t i = 0; i < clients_to_remove.size(); ++i) {
        removeClient(clients_to_remove[i]);
    }
}
// فـ server.cpp
void Server::handleClientEvent() {
    std::vector<int> clients_to_remove;
    
    std::map<int, Client *>::iterator it;
    for (it = clients.begin(); it != clients.end(); ++it) {
        int client_fd = it->first;
        Client* client = it->second;
        uint32_t events = notifEvent[client_fd];

        if (events & (EPOLLHUP | EPOLLERR)) {
            clients_to_remove.push_back(client_fd);
            continue;
        }

        // الخطوة 1: يلا كاين ما يتقرا، كنقراوه
        if (events & EPOLLIN) {
            client->readRequest();
        }

        // الخطوة 2: كنعطيو للعقل المدبر الجديد يدير خدمتو
        client->manageState();

        if (client->isDone()) {
            clients_to_remove.push_back(client_fd);
        }
    }
    
    for (size_t i = 0; i < clients_to_remove.size(); ++i) {
        removeClient(clients_to_remove[i]);
    }
    // كنمسحو ال notifEvent باش منعاودوش نعالجو نفس الحدث
    notifEvent.clear();
}
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
//         // removeClient(client_fd);
//     }

// }

void    Server::eventLoop(){
    struct epoll_event events_received[MAX_EVENTS];
    int EVENT = -1;
    while (true)
    {
        int num_events = epoll_wait(epoll_fd, events_received, MAX_EVENTS, EVENT);
        for(int i = 0; i < num_events; i++)
        {
            int active_fd = events_received[i].data.fd;
            std::vector<int>::iterator it = std::find(listening_fds.begin(), listening_fds.end(), active_fd);
            if (it != listening_fds.end()) {
                handleNewConnection(active_fd);
            } else {
                notifEvent[active_fd] = events_received[i].events;
            }
        }
        handleClientEvent();
        if(clients.size())
            EVENT = 0;
        else
            EVENT = -1;
        checkTimeouts();

    }
    
}
    void    Server::run(){
    setupServer();
    eventLoop();
}