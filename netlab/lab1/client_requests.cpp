#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <vector>
#include "client_requests.h"
#include "io.h"
using namespace std;

extern int sock;


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

    // 1.读取固定长度的 Header
    if (!recv_all(sock, &hs, sizeof(hs)) || !check_header(hs, LIST_REPLY)) {
        return;
    }
    // 2.读取payload
    const uint32_t total_len = ntohl(hs.length);
    const size_t payload_len = total_len - sizeof(hs);
    vector<char> payload(payload_len);

    if (!recv_all(sock, payload.data(), payload.size())) {
        fprintf(stderr, "Failed to receive LIST_REPLY payload\n");
    }
    if (payload.back() != '\0') {
        fprintf(stderr, "LIST_REPLY is not null-terminated\n");
    }

    // 3.输出到stdout，不输出最后的 '\0'
    fwrite(payload.data(), 1, payload.size() - 1, stdout);
}


void get_request(struct Header& hc, const char* filename){
    // Request
    hc.length = htonl(12 + strlen(filename) + 1);
    send_all(sock, &hc, sizeof(hc));
    send_all(sock, filename, strlen(filename) + 1);
    printf("Client sent GET_REQUEST for file: %s\n", filename);

    // Reply
    struct Header hs{};
    if (!recv_all(sock, &hs, sizeof(hs)) || !check_header(hs, GET_REPLY)) {
        return;
    }
    if(hs.status == 0){
        printf("File %s not found on server\n", filename);
        return;
    }

    // file exists, receive FILE_DATA
    struct Header hs_data{};
    if (!recv_all(sock, &hs_data, sizeof(hs_data)) ||
        !check_header(hs_data, FILE_DATA)) {
        return;
    }

    const uint32_t total_len = ntohl(hs_data.length);
    if (total_len < sizeof(hs_data)) {
        fprintf(stderr, "Invalid FILE_DATA length\n");
        return;
    }

    size_t remaining = total_len - sizeof(hs_data);
    FILE* file = fopen(filename, "wb"); // wb: write binary，不存在就创建、覆盖写
    if (file == nullptr) {
        perror("fopen");
    }

    char buffer[4096]; //按chunk下载文件
    while (remaining > 0) {
        const size_t chunk_size =
            remaining < sizeof(buffer) ? remaining : sizeof(buffer);
        if (!recv_all(sock, buffer, chunk_size)) {
            fprintf(stderr, "Failed to receive FILE_DATA payload\n");
            if (file != nullptr) {
                fclose(file);
            }
            return;
        }
        if (file != nullptr) {
            fwrite(buffer, 1, chunk_size, file);
        }
        remaining -= chunk_size;
    }
    if (file != nullptr) {
        fclose(file);
        printf("Client received file: %s\n", filename);
    }
}
