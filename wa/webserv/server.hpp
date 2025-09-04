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
#define TIMEOUT_SECONDS 5 // مدة الخمول المسموح بها (60 ثانية)

class Server {
public:
    Server();
    Server(const Server& src);
    Server& operator=(const Server& rhs);
    ~Server();

    Server(const ConfigFile& _config);

    void run();

private:
    const ConfigFile&   config;
    int                 epoll_fd;
    std::vector<int>    listening_fds;
   // فـ server.hpp
    std::map<int, Client*> clients;
    std::map<int, uint32_t> notifEvent;

    void setupServer();
     void checkTimeouts(); // <== زيد هادي هنا
    void eventLoop();
    void handleNewConnection(int listener_fd);
    void handleClientEvent();
    void removeClient(int client_fd);
};