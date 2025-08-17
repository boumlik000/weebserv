#include "wb0.hpp"

void EpollServer::run() 
{
    if (!setupServer()) {
        return;
    }
    
    
    struct epoll_event events[MAX_EVENTS];
    
    while (true) {
        // Wait for events
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, 1000); // 1 second timeout
        
        if (num_events == -1) {
            std::cerr << "❌ Epoll wait error: " << strerror(errno) << std::endl;
            break;
        }
        
        if (num_events == 0) {
            // Timeout - could do maintenance here
            static int heartbeat = 0;
            if (++heartbeat % 10 == 0) {  // Every 10 seconds
                std::cout << "💓 Server heartbeat - Clients: " << client_fds.size() << std::endl;
            }
            continue;
        }
        
        // Process all events
        for (int i = 0; i < num_events; ++i) {
            int fd = events[i].data.fd;
            
            if (fd == server_fd) {
                // New connection on server socket
                handleNewClient();
                
            } else {
                // Data on client socket
                if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                    // Error or hangup
                    std::cout << "❌ Error/Hangup on client " << fd << std::endl;
                    removeClient(fd);
                } else if (events[i].events & EPOLLIN) {
                    // Data available for reading
                    handleClientData(fd);
                }
            }
        }
    }
}