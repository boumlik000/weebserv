#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <algorithm>
#include <cstring>  // For strerror
#include <cerrno>   // For errno

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <sys/types.h>
    #include <ifaddrs.h>
#endif

class FirewallPortManager {
private:
    std::map<int, std::string> common_services = {
        {21, "FTP"}, {22, "SSH"}, {23, "Telnet"}, {25, "SMTP"},
        {53, "DNS"}, {80, "HTTP"}, {110, "POP3"}, {143, "IMAP"},
        {443, "HTTPS"}, {993, "IMAPS"}, {995, "POP3S"},
        {135, "RPC"}, {139, "NetBIOS"}, {445, "SMB"},
        {1433, "MSSQL"}, {3389, "RDP"}, {5432, "PostgreSQL"},
        {3306, "MySQL"}, {6379, "Redis"}, {8080, "HTTP-Alt"},
        {8443, "HTTPS-Alt"}, {9000, "HTTP-Alt2"}
    };
    
    std::vector<int> listening_ports;
    std::vector<int> blocked_ports;
    std::vector<int> available_ports;

public:
    FirewallPortManager() {
#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    }
    
    ~FirewallPortManager() {
#ifdef _WIN32
        WSACleanup();
#endif
    }

    // Check what ports are currently listening on the system
    void checkListeningPorts() {
        listening_ports.clear();
        std::cout << "Checking listening ports...\n";
        
#ifdef _WIN32
        system("netstat -an | findstr LISTENING > listening_ports.txt");
#else
        system("netstat -tuln | grep LISTEN > listening_ports.txt");
#endif
        
        std::ifstream file("listening_ports.txt");
        std::string line;
        
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string protocol, recv_q, send_q, local_addr, foreign_addr, state;
            
            if (iss >> protocol >> recv_q >> send_q >> local_addr >> foreign_addr >> state) {
                size_t pos = local_addr.find_last_of(':');
                if (pos != std::string::npos) {
                    try {
                        int port = std::stoi(local_addr.substr(pos + 1));
                        if (std::find(listening_ports.begin(), listening_ports.end(), port) == listening_ports.end()) {
                            listening_ports.push_back(port);
                        }
                    } catch (...) {
                        // Invalid port number, skip
                    }
                }
            }
        }
        file.close();
        system("rm -f listening_ports.txt"); // Clean up temp file
        
        std::sort(listening_ports.begin(), listening_ports.end());
    }

    // Test if a port can be bound (available for use)
    bool testPortAvailability(int port) {
#ifdef _WIN32
        SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) return false;
#else
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) return false;
        
        int opt = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif
        
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr("197.230.240.146");
        
        bool available = (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0);
        
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        
        return available;
    }

    // Find available ports in a range
    void findAvailablePorts(int start = 1024, int end = 65535, int max_ports = 100) {
        available_ports.clear();
        std::cout << "Finding available ports in range " << start << "-" << end << "...\n";
        
        int found = 0;
        for (int port = start; port <= end && found < max_ports; ++port) {
            if (testPortAvailability(port)) {
                available_ports.push_back(port);
                found++;
                if (found % 10 == 0) {
                    std::cout << "Found " << found << " available ports...\n";
                }
            }
        }
        
        std::cout << "Found " << available_ports.size() << " available ports.\n";
    }

    // Generate firewall rules for different systems
    void generateFirewallRules(const std::vector<int>& ports_to_open) {
        std::cout << "\n=== FIREWALL RULES ===\n";
        
        // UFW (Ubuntu/Debian)
        std::cout << "\n--- UFW Rules (Ubuntu/Debian) ---\n";
        for (int port : ports_to_open) {
            std::cout << "sudo ufw allow " << port << std::endl;
        }
        
        // iptables (Generic Linux)
        std::cout << "\n--- iptables Rules (Generic Linux) ---\n";
        for (int port : ports_to_open) {
            std::cout << "sudo iptables -A INPUT -p tcp --dport " << port << " -j ACCEPT" << std::endl;
        }
        
        // firewall-cmd (RHEL/CentOS/Fedora)
        std::cout << "\n--- firewall-cmd Rules (RHEL/CentOS/Fedora) ---\n";
        for (int port : ports_to_open) {
            std::cout << "sudo firewall-cmd --permanent --add-port=" << port << "/tcp" << std::endl;
        }
        std::cout << "sudo firewall-cmd --reload" << std::endl;
        
        // Windows Firewall
        std::cout << "\n--- Windows Firewall Rules ---\n";
        for (int port : ports_to_open) {
            std::cout << "netsh advfirewall firewall add rule name=\"Allow Port " << port 
                     << "\" dir=in action=allow protocol=TCP localport=" << port << std::endl;
        }
    }

    // Find first available port in range
    int findFirstAvailablePort(int start = 8000, int end = 9000) {
        for (int port = start; port <= end; ++port) {
            if (testPortAvailability(port)) {
                return port;
            }
        }
        return -1; // No available port found
    }

    // Create a simple port tester server
    void createPortTester(int port) {
        std::cout << "Creating port tester on port " << port << "...\n";
        
#ifdef _WIN32
        SOCKET server_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (server_sock == INVALID_SOCKET) {
#else
        int server_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (server_sock < 0) {
#endif
            std::cerr << "Failed to create socket\n";
            return;
        }
        
#ifndef _WIN32
        int opt = 1;
        setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif
        
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = INADDR_ANY; // Bind to all interfaces
        
        if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "Failed to bind to port " << port << ": " << strerror(errno) << std::endl;
#ifdef _WIN32
            closesocket(server_sock);
#else
            close(server_sock);
#endif
            return;
        }
        
        if (listen(server_sock, 5) < 0) {
            std::cerr << "Failed to listen on port " << port << std::endl;
#ifdef _WIN32
            closesocket(server_sock);
#else
            close(server_sock);
#endif
            return;
        }
        
        std::cout << "🎉 Port tester listening on port " << port << std::endl;
        std::cout << "🌐 Test with: curl http://localhost:" << port << std::endl;
        std::cout << "🌐 Or open browser: http://localhost:" << port << std::endl;
        std::cout << "⏹️  Press Ctrl+C to stop...\n";
        
        while (true) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            
#ifdef _WIN32
            SOCKET client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
            if (client_sock != INVALID_SOCKET) {
#else
            int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
            if (client_sock >= 0) {
#endif
                std::cout << "Connection received from " << inet_ntoa(client_addr.sin_addr) << std::endl;
                
                std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
                                     "<html><body><h1>Port " + std::to_string(port) + " Test Successful!</h1>"
                                     "<p>Your epoll server can use this port!</p></body></html>";
                
                send(client_sock, response.c_str(), response.length(), 0);
                
#ifdef _WIN32
                closesocket(client_sock);
#else
                close(client_sock);
#endif
            }
        }
    }

    void printListeningPorts() {
        std::cout << "\n=== CURRENTLY LISTENING PORTS ===\n";
        for (int port : listening_ports) {
            std::cout << "Port " << port;
            if (common_services.find(port) != common_services.end()) {
                std::cout << " (" << common_services[port] << ")";
            }
            std::cout << std::endl;
        }
    }

    void printAvailablePorts(int limit = 20) {
        std::cout << "\n=== AVAILABLE PORTS (First " << limit << ") ===\n";
        int count = 0;
        for (int port : available_ports) {
            if (count >= limit) break;
            std::cout << "Port " << port << std::endl;
            count++;
        }
    }

    std::vector<int> getAvailablePorts() const {
        return available_ports;
    }

    std::vector<int> getListeningPorts() const {
        return listening_ports;
    }
};

int main() {
    FirewallPortManager manager;
    int choice;
    
    std::cout << "Firewall and Port Management System\n";
    std::cout << "===================================\n";
    
    while (true) {
        std::cout << "\nSelect option:\n";
        std::cout << "1. Check currently listening ports\n";
        std::cout << "2. Find available ports\n";
        std::cout << "3. Generate firewall rules\n";
        std::cout << "4. Test specific port\n";
        std::cout << "5. Create port tester server\n";
        std::cout << "6. Exit\n";
        std::cout << "Enter choice (1-6): ";
        std::cin >> choice;
        
        switch (choice) {
            case 1:
                manager.checkListeningPorts();
                manager.printListeningPorts();
                break;
                
            case 2: {
                int start, end, max_ports;
                std::cout << "Enter start port (default 1024): ";
                std::cin >> start;
                std::cout << "Enter end port (default 65535): ";
                std::cin >> end;
                std::cout << "Max ports to find (default 100): ";
                std::cin >> max_ports;
                
                manager.findAvailablePorts(start, end, max_ports);
                manager.printAvailablePorts();
                break;
            }
            
            case 3: {
                auto available = manager.getAvailablePorts();
                if (available.empty()) {
                    std::cout << "Please find available ports first (option 2)\n";
                    break;
                }
                
                std::vector<int> ports_to_open;
                std::cout << "Enter ports to open (space-separated, 0 to finish): ";
                int port;
                while (std::cin >> port && port != 0) {
                    ports_to_open.push_back(port);
                }
                
                manager.generateFirewallRules(ports_to_open);
                break;
            }
            
            case 4: {
                int port;
                std::cout << "Enter port to test: ";
                std::cin >> port;
                
                if (manager.testPortAvailability(port)) {
                    std::cout << "Port " << port << " is AVAILABLE for binding\n";
                } else {
                    std::cout << "Port " << port << " is NOT AVAILABLE (already in use)\n";
                }
                break;
            }
            
            case 5: {
                int port;
                std::cout << "Enter port for test server: ";
                std::cin >> port;
                
                std::cout << "Starting port tester server...\n";
                manager.createPortTester(port);
                break;
            }
            
            case 6:
                std::cout << "Goodbye!\n";
                return 0;
                
            default:
                std::cout << "Invalid choice\n";
        }
    }
    
    return 0;
}