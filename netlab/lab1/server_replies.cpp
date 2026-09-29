#include "server_replies.h"

#include <arpa/inet.h>

#include <cstdio>
#include <string>

#include "io.h"
#include "protocol.h"

// OPEN_CONN_REPLY
void open_conn_reply(int sock){
    struct Header hs{};
    hs.type = OPEN_CONN_REPLY;
    hs.status = 1;
    hs.length = htonl(12);
    send_all(sock, &hs, sizeof(hs));
    printf("Server sent OPEN_CONN_REPLY\n");
}

// LIST_REPLY
void list_reply(int client_sock)
{
    FILE* pipe = popen("ls", "r");
    if (pipe == nullptr) {
        perror("popen");
    }

    std::string result;
    char buffer[512];

    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }

    if (pclose(pipe) == -1) {
        perror("pclose");
    }
    Header reply{};
    reply.type = LIST_REPLY;
    reply.status = 0;

    // +1 是字符串结尾的 '\0'
    const uint32_t total_length =
        sizeof(Header) + result.size() + 1;

    reply.length = htonl(total_length);

    // TCP 可以分两次发送，接收端按 length 组装
    send_all(client_sock, &reply, sizeof(reply));
    send_all(client_sock, result.c_str(), result.size() + 1);

    printf("Server sent LIST_REPLY\n");
}
