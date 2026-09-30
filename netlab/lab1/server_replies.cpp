#include <arpa/inet.h>
#include <cstdio>
#include <string>
#include "server_replies.h"

#include "protocol.h"
#include "io.h"

using namespace std;


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

    const uint32_t total_length =
        sizeof(Header) + result.size() + 1; // +1:字符串结尾的 '\0'
    reply.length = htonl(total_length);

    // TCP 可以分两次发送，接收端按 length 组装
    send_all(client_sock, &reply, sizeof(reply));
    send_all(client_sock, result.c_str(), result.size() + 1);

    printf("Server sent LIST_REPLY\n");
}


// GET_REPLY
void get_reply(int sock)
{
    // Receive filename payload
    string filename;
    char ch;
    do {
        if (!recv_all(sock, &ch, sizeof(ch))) {
            return;
        }
        if (ch != '\0') {
            filename += ch;
        }
    } while (ch != '\0');

    FILE* file = fopen(filename.c_str(), "rb"); // rb: read binary,只读

    Header reply{};
    reply.type = GET_REPLY;
    reply.status = 0;
    reply.length = htonl(sizeof(Header));

    // file does not exist, send GET_REPLY with status 0
    if (file == nullptr) {
        send_all(sock, &reply, sizeof(reply));
        printf("Server sent GET_REPLY: file not found\n");
        return;
    }

    // 移动指针到末尾，获取文件大小，再移回开头
    fseek(file, 0, SEEK_END);
    const long file_size = ftell(file);
    rewind(file);
    if (file_size < 0) {
        perror("ftell");
        fclose(file);
        send_all(sock, &reply, sizeof(reply));
        return;
    }

    // file exists, send GET_REPLY with status 1
    reply.status = 1;
    send_all(sock, &reply, sizeof(reply));

    // send FILE_DATA header
    struct Header hs_data{};
    hs_data.type = FILE_DATA;
    hs_data.status = 0;
    hs_data.length = htonl(sizeof(Header) + file_size);
    send_all(sock, &hs_data, sizeof(hs_data));

    // send file payload in chunks
    char buffer[4096];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        send_all(sock, buffer, bytes_read);
    }

    if (ferror(file)) {
        perror("fread");
    }

    fclose(file);

    printf("Server sent GET_REPLY and FILE_DATA\n");
}


void put_reply(int sock)
{
    // Receive filename payload
    string filename;
    char ch;
    do {
        if (!recv_all(sock, &ch, sizeof(ch))) {
            return;
        }
        if (ch != '\0') {
            filename += ch;
        }
    } while (ch != '\0');

    // Reply
    struct Header hs{};
    hs.type = PUT_REPLY;
    hs.length = htonl(12);
    send_all(sock, &hs, sizeof(hs));
    printf("Server sent PUT_REPLY for file: %s\n", filename.c_str());


    // Receive file_data header
    struct Header hc_data{};
    if (!recv_all(sock, &hc_data, sizeof(hc_data)) ||
        !check_header(hc_data, FILE_DATA)) {
        return;
    }

    const uint32_t total_len = ntohl(hc_data.length);
    if (total_len < sizeof(hc_data)) {
        fprintf(stderr, "Invalid FILE_DATA length\n");
        return;
    }

    size_t remaining = total_len - sizeof(hc_data);
    FILE* file = fopen(filename.c_str(), "wb"); // wb: write binary，不存在就创建、覆盖写
    if (file == nullptr) {
        perror("fopen");
    }

    // Download file in chunks
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
        printf("Server received file: %s\n", filename.c_str());
    }
}
