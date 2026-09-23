#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include "protocol.h"

int main() {
    std::cerr << "hello from ftp server" << std::endl;

    // 1. Socket
    int sock = socket(AF_INET, SOCK_STREAM, 0); // (domain, type, protocol)
    struct sockaddr_in addr; // 描述监听的地址
    addr.sin_port = htons(12323); // 在12323端口监听, htons是host to network (short)的简称，表示进行大小端表示法转换，网络中一般使用大端法
    addr.sin_family = AF_INET; // 表示使用AF_INET地址族
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr); // 监听127.0.0.1地址，将字符串表示转化为二进制表示

    // 2. Bind
    bind(sock, (struct sockaddr*)&addr, sizeof(addr)); // addr = ip + port

    // 3. Listen
    listen(sock, 128); // 128: pending connection backlog

    // 4. Accept connection
    int client = accept(sock, nullptr, nullptr);
    printf("Accepted a client\n");

    // 5. Receive a request
    char buffer[128];
    size_t l = recv(client, buffer, 128, 0);
    printf("Server received\n");

    // 6. Send a reply
    struct Header h{};
    h.type = OPEN_CONN_REPLY;
    h.status = 1; 
    h.length = htonl(12);
    send(client, &h, sizeof(h), 0);
    printf("Server sent\n");

    return 0;
}
