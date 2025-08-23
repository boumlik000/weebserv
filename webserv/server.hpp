#pragma once
#include <sys/epoll.h>
#include <iostream>
#include <vector>
#include <map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h> // For fcntl
#include "ConfigFile.hpp" // الملف لي فيه الكلاس ديال الكونفيغ ديالك
#include "client.hpp"     // كلاس أخرى باش تدير gestion لكل client بوحدو
#define MAX_EVENTS 64

class Server {
public:
    Server();
    Server(const Server& src);
    Server& operator=(const Server& rhs);
    ~Server();


    void run();

private:
    int                 epoll_fd;
    std::vector<int>    listening_fds;
    std::map<int, Client> clients;

    void setupServer();
    void eventLoop();
    void handleNewConnection(int listener_fd);
    void handleClientEvent(int client_fd);
    void removeClient(int client_fd);
};