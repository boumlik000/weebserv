#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <vector>
#include <fcntl.h> // For fcntl
#include <string.h>
#include <errno.h>

#define PORT 8080
#define MAX_EVENTS 10

int main() {
    // 1. Setup Listening Socket (non-blocking)
    int listener_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (listener_fd < 0) {
        //perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(listener_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        //perror("bind");
        return 1;
    }

    if (listen(listener_fd, 10) < 0) {
        //perror("listen");
        return 1;
    }
    std::cout << "[INFO] Server listening on port " << PORT << std::endl;

    // 2. Setup Epoll
    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        //perror("epoll_create1");
        return 1;
    }

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = listener_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listener_fd, &event) < 0) {
        //perror("epoll_ctl");
        return 1;
    }

    struct epoll_event events[MAX_EVENTS];

    // 3. Event Loop
    while (true) {
        int n_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (int i = 0; i < n_events; ++i) {
            if (events[i].data.fd == listener_fd) {

                if (client_fd < 0) { /* ... */ }
                std::cout << "[INFO] New connection accepted with fd: " << client_fd << std::endl;

                // --- بدا كود التشخيص ---
                // fcntl(client_fd, F_SETFL, O_NONBLOCK);
                int flags = fcntl(client_fd, F_GETFL, 0); // F_GETFL كتعني "جيب ليا الفلاغز"
                if (flags == -1) {
                    //perror("fcntl F_GETFL");
                }

                if (flags & O_NONBLOCK) {
                    std::cout << "[DEBUG] The new client socket IS non-blocking!" << std::endl;
                } else {
                    std::cout << "[DEBUG] The new client socket IS BLOCKING." << std::endl;
                }
                // --- سالا كود التشخيص ---

                event.events = EPOLLIN;
                event.data.fd = client_fd;
                // ... السطر لي من بعد
                

                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) < 0) {
                    //perror("epoll_ctl add client");
                }
           // ...
        // ...
    } else {
        // Data from existing client
        int client_fd = events[i].data.fd;
        char buffer[1024] = {0};
        ssize_t total_bytes_read = 0;

        std::cout << "[DANGEROUS HANDLER] Trying to read until we get 50 bytes..." << std::endl;

        // هاد الحلقة هي لي غادي تجمد السيرفر
        while (total_bytes_read < 50) {
            ssize_t bytes_read_now = recv(client_fd, buffer + total_bytes_read, 1024 - total_bytes_read, 0);
            // std::cout << "[DANGEROUS HANDLER] Calling recv() again... (total received so far: " << total_bytes_read << ")" << std::endl;
            // std::cout << "fooot" << std::endl;
            // if(bytes_read_now < 0)
                //perror("error : ");

            if (bytes_read_now > 0) {
                total_bytes_read += bytes_read_now;
            } else if (bytes_read_now == 0) {
                std::cout << "[DANGEROUS HANDLER] Client disconnected." << std::endl;
                close(client_fd);
                // اخرج من الحلقة حيت الكليان مشا
                total_bytes_read = -1; // Just to break the outer loop
                break; 
            } else { // bytes_read_now == -1
                // على سوكت بلوكينغ، مستحيل يوصل لهنا بلا ما يجمد الفوق
                // ولكن يلا وصل، يعني وقع خطأ
                //perror("recv in loop");
                close(client_fd);
                total_bytes_read = -1; // Just to break the outer loop
                break;
            }
        }
        
        if (total_bytes_read != -1) {
            std::cout << "[DANGEROUS HANDLER] Finally finished reading. Total: " << total_bytes_read << std::endl;
        }
    }
// ...
// ...
        }
    }

    return 0;
}