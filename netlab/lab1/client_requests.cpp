#include "client_requests.h"

#include <arpa/inet.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <vector>

#include "io.h"

extern int sock;

constexpr unsigned char kProtocolMagic[6] = {
    0xc1, 0xa1, 0x10, 'f', 't', 'p'
};

// OPEN_CONN_REQUEST
bool open_conn_request(){
    // Request
    struct Header hc{};
    hc.type = OPEN_CONN_REQUEST;
    hc.length = htonl(12);
    // 把只有header的报文写进buf发给服务器
    send_all(sock, &hc, sizeof(hc));
    printf("Client sent OPEN_CONN_REQUEST\n");

    // Reply
    struct Header hs{};
    if (!recv_all(sock, &hs, sizeof(hs), 1000)) {
        if (errno == ETIMEDOUT) { // 端口被监听（TCP成功），但不是被MyFTP Server进程
            fprintf(stderr, "OPEN_CONN_REPLY timeout after 1 second\n");
        } else {
            fprintf(stderr, "Failed to receive OPEN_CONN_REPLY\n");
        }
        return false;
    }
    if(hs.type == OPEN_CONN_REPLY && hs.status == 1){
        printf("Connection accepted\n");
        return true;
    } else {
        printf("Connection refused, idle\n");
        return false;
    }
}

// LIST_REQUEST
void list_request(struct Header& hc){
    // Request
    hc.length = htonl(12);
    send(sock, &hc, sizeof(hc), 0);
    printf("Client sent LIST_REQUEST\n");

    // Reply
    struct Header hs{};

    // 先读取固定长度的 Header
    // TODO: 包装成检查header一致函数
    if (!recv_all(sock, &hs, sizeof(hs))) {
        fprintf(stderr, "Failed to receive LIST_REPLY\n");
    }
    if (memcmp(hc.protocol, kProtocolMagic,
                    sizeof(kProtocolMagic)) != 0) {
        std::cerr << "Invalid protocol magic\n";
    }
    if (hs.type != LIST_REPLY) {
        fprintf(stderr, "Expected LIST_REPLY, got 0x%02X\n",
                hs.type);
    }

    const uint32_t total_len = ntohl(hs.length);

    const size_t payload_len =
        total_len - sizeof(hs);

    std::vector<char> payload(payload_len);

    if (!recv_all(sock, payload.data(), payload.size())) {
        fprintf(stderr, "Failed to receive LIST_REPLY payload\n");
    }

    if (payload.back() != '\0') {
        fprintf(stderr, "LIST_REPLY is not null-terminated\n");
    }

    // 不输出最后的 '\0'
    fwrite(payload.data(), 1, payload.size() - 1, stdout);
}
