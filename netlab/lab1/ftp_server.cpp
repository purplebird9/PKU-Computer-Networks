#include <iostream>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include "protocol.h"
#include "io.h"
using namespace std;

constexpr unsigned char kProtocolMagic[6] = {
    0xc1, 0xa1, 0x10, 'f', 't', 'p'
};

void open_conn_reply(int sock);

void* handle_client(void* arg) {
    int* p = static_cast<int*>(arg);
    int c_sock = *p; // Connection socket fd
    delete p;
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
        struct Header hs{};
        switch (hc.type) {
        case OPEN_CONN_REQUEST:
            //if (!send_header(session.fd, OPEN_CONN_REPLY,
            //                 session.established ? 0 : 1)) {
            //    close(session.fd);
            //    return;
            //}
            //session.established = true;
            open_conn_reply(c_sock);
            break;

        case QUIT_REQUEST:
            connected = false;
            break;

        default:
            // other requests
            break;
        }
    }

    close(c_sock);
    return nullptr;
}

int main() {
    std::cerr << "hello from ftp server" << std::endl;
    /* ========================= Main Thread =========================*/
    // 1. Socket
    int s_sock = socket(AF_INET, SOCK_STREAM, 0); // (domain, type, protocol)
    struct sockaddr_in addr; // 描述监听的地址
    addr.sin_port = htons(12323); // 在12323端口监听, htons是host to network (short)的简称，表示进行大小端表示法转换，网络中一般使用大端法
    addr.sin_family = AF_INET; // 表示使用AF_INET地址族
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr); // 监听127.0.0.1地址，将字符串表示转化为二进制表示

    // 2. Bind
    bind(s_sock, (struct sockaddr*)&addr, sizeof(addr)); // addr = ip + port

    // 3. Listen
    listen(s_sock, 128); // 128: pending connection backlog


    while (true) {
        // 4. Accept a client
        int* c_sock = new int(accept(s_sock, nullptr, nullptr));

        /*====================== Child Thread =======================*/
        pthread_t thread;
        pthread_create(&thread, nullptr, handle_client, c_sock);
        pthread_detach(thread);
    }

    return 0;
}

/* ========================= Helpers ==========================*/

// OPEN_CONN_REPLY
void open_conn_reply(int sock){
    struct Header hs{};
    hs.type = OPEN_CONN_REPLY;
    hs.status = 1;
    hs.length = htonl(12); 
    send_all(sock, &hs, sizeof(hs)); 
    printf("Server sent OPEN_CONN_REPLY\n");
}
