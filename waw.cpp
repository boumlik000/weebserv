#include <iostream>
#include <fstream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(65001);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    // قراءة الصورة فـ buffer
    std::ifstream file("/home/rlamtaou/Desktop/webserver/webserv/pages/home/project.jpg", std::ios::binary);
    std::string body((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    size_t body_size = body.size();

    // بناء headers يدويًا
    std::string request = "DELETE /de.html HTTP/1.1\r\n";
    request += "Host: localhost:65001\r\n";
    request += "Content-Type: image/jpeg\r\n";
    request += "Content-Length: "+ std::to_string(10) + "\r\n";
    request += "\r\n"; // نهاية headers

    // إرسال headers
    send(sock, request.c_str(), request.size(), 0);
    // إرسال body
    send(sock, body.c_str(), body_size, 0);

    // قراءة response
    char buffer[4096];
    int n = recv(sock, buffer, sizeof(buffer)-1, 0);
    buffer[n] = '\0';
    std::cout << buffer;

    close(sock);
}
