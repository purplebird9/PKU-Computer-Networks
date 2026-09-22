#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include "protocol.h"

int main() {
    std::cerr << "hello from ftp client" << std::endl;
    
    // 1. Parse command
    // 2. Socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_port = htons(12323); 
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr); // 表示我们要连接到服务器的127.0.0.1:12323

    // 3. Try connect
    printf("Try connect\n");
    connect(sock, (struct sockaddr*)&addr, sizeof(addr));

    // 4. Send a request
    char buffer[128];
    sprintf(buffer, "Hello Server");
    send(sock, buffer, strlen(buffer)+1, 0);
    printf("Client sent\n");

    // 5. Receive a reply
    recv(sock, buffer, 128, 0);
    printf("Client received\n");
    // 6. Close connection


    return 0;

}