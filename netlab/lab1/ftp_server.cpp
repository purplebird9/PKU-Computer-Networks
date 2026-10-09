#include <iostream>
#include <cstdio>
#include <filesystem>
#include <string>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include "protocol.h"
#include "io.h"
#include "server_replies.h"
using namespace std;

struct ClientSession {
    int sock;
    filesystem::path current_dir;
    ClientSession(int s, const filesystem::path& dir) : sock(s), current_dir(dir) {}
};

// 服务一个client的thread函数
void* handle_client(void* arg) {
    ClientSession* session = static_cast<ClientSession*>(arg);
    int c_sock = session->sock; // Connection socket fd
    filesystem::path cwd = session->current_dir;// 这个client视角下的cwd
    delete session;
    printf("Accepted a client(fd=%d)\n",c_sock);


    bool connected = true;
    // Server Loop
    while(connected){    
        // Receive a request
        struct Header hc{};
        if (!recv_all(c_sock, &hc, sizeof(hc))) {
            break; //避免client断开后不断空转
        }

        if (memcmp(hc.protocol, kProtocolMagic,
                        sizeof(kProtocolMagic)) != 0) {
            cerr << "Invalid protocol magic\n";
            return nullptr;
        }
        printf("Server received request 0x%02X\n", hc.type);

        // Send a reply
        switch (hc.type) {
        case OPEN_CONN_REQUEST:
            open_conn_reply(c_sock);
            break;
        case LIST_REQUEST:
            list_reply(c_sock, cwd);
            break;
        case CHANGE_DIR_REQUEST:
            change_dir_reply(c_sock, cwd);
            break;
        case GET_REQUEST:
            get_reply(c_sock, cwd);
            break;
        case PUT_REQUEST:
            put_reply(c_sock, cwd);
            break;
        case SHA_REQUEST:
            break;
        case QUIT_REQUEST:
            connected = false;
            break;

        default:
            break;
        }
    }

    close(c_sock);
    return nullptr;
}

int main(int argc, char* argv[]) {
    std::cerr << "hello from ftp server" << std::endl;

    /* ========================= Main Thread =========================*/
    const char* ip = argv[1];
    int port = stoi(argv[2]);

    // 1. Socket
    int s_sock = socket(AF_INET, SOCK_STREAM, 0); // (domain, type, protocol)
    struct sockaddr_in addr; // 描述监听的地址
    addr.sin_port = htons(port); // 在12323端口监听, htons是host to network (short)的简称，表示进行大小端表示法转换，网络中一般使用大端法
    addr.sin_family = AF_INET; // 表示使用AF_INET地址族
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr); // 监听127.0.0.1地址，将字符串表示转化为二进制表示

    // 2. Bind
    bind(s_sock, (struct sockaddr*)&addr, sizeof(addr)); // addr = ip + port

    // 3. Listen
    listen(s_sock, 128); // 128: pending connection backlog


    filesystem::path server_dir = filesystem::current_path();
    while (true) {
        // 4. Accept a client
        int c_sock = accept(s_sock, nullptr, nullptr);
        ClientSession* session = new ClientSession(c_sock, server_dir);

        /*====================== Child Thread =======================*/
        pthread_t thread;
        pthread_create(&thread, nullptr, handle_client, session);
        pthread_detach(thread);
    }

    return 0;
}
