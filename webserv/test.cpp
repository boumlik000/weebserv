#include <iostream>
#include <sys/socket.h> // For socket(), bind(), listen(), accept()
#include <netinet/in.h> // For sockaddr_in
#include <unistd.h>     // For close()
#include <string.h>     // For close()

#define PORT 8080
using namespace std;
int main(){
    int server = socket(AF_INET, SOCK_STREAM, 0);// hna sayabt socket li tkhdam f internet w b method tcp
    if(server < 0)
    {
        cerr<<"can't creat socket"<<endl;
        return 1;
    }

    sockaddr_in address;
    address.sin_family = AF_INET; // IPV_4
    address.sin_addr.s_addr = INADDR_ANY; // Listen on 0.0.0.0 (all available interfaces)
    address.sin_port = htons(PORT);       // Host To Network Short

    if(bind(server, (struct sockaddr *)&address, sizeof(address)) < 0)// katrbat socket b ip w port bach tasta9bal ga3 connections
    {
        cerr<<"can't bind the server"<<endl;
        close(server);
        return 1;
    }

    if(listen(server, 5) < 0) // hadi katb9a tsana connection request mn clients max client hna howa 5
    {
        cerr<<"listen failed"<<endl;
        close(server);
        return 1;
    }
    cout<<"server listening on PORT 8080"<<endl;

    int lenAddress = sizeof(address);
    int client = accept(server, (struct sockaddr *)&address, (socklen_t *)&lenAddress);// hna sayabna socket jdida li hia client fhad l7ala aka darna connetion bin server w client
    if(client < 0)
    {
        cerr<<"connection failed with the client"<<endl;
        close(server);
        return 1;
    } 

    cout<<"connection accepted with the client"<<endl;

    char request[1024];
    recv(client, request, 1024, 0);//chadit request mn client
    cout<<"request : "<<endl;
    cout<< request <<endl;

    const char * respons = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 12\r\n\r\nHello World!";
    send(client, respons, strlen(respons), 0);//sifat respons mn client

    cout<<"respons sended"<<endl;


    close(server);
    close(client);
}