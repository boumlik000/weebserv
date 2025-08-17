#ifndef WB0_HPP
#define WB0_HPP

#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <vector>
#include <errno.h>

#define MAX_EVENTS 10
#define PORT 8080
#define BUFFER_SIZE 1024

class EpollServer {
    private:
        int server_fd;
        int epoll_fd;
        std::vector<int> client_fds;
        
    public:
        EpollServer() : server_fd(-1), epoll_fd(-1) {}
        
        ~EpollServer() {
            cleanup();
        }
        
        void cleanup() {
            for (size_t i = 0; i < client_fds.size(); ++i) {
                close(client_fds[i]);
            }
            if (epoll_fd != -1) close(epoll_fd);
            if (server_fd != -1) close(server_fd);
        }
        
        bool setNonBlocking(int fd) {
            int flags = fcntl(fd, F_GETFL, 0);
            if (flags == -1) return false;
            return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
        }
        
        bool setupServer();
        void handleNewClient();
        void handleClientData(int client_fd) ;
        void removeClient(int client_fd);
        void run();
};
#endif // !
