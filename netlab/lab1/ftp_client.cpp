#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <sstream>
#include "protocol.h"
#include "io.h"

using namespace std;

int sock = -1;

bool open_conn_request();
void list_request();

int main() {
    cerr << "hello from ftp client" << endl;

    string command;
    string argument;
    string ip;
    unsigned short port = 0;
    MessageType request_type;
    bool established = false;

    /* ====================== IDLE LOOP ========================*/
    string line;
    istringstream command_line;
    while (!established) {
        printf("Client(none)>");
        fflush(stdout);
        if (!getline(cin, line)) {
            return 0;
        }

        command_line.clear();
        command_line.str(line);
        command_line >> command;

        if (command == "quit") {
            return 0;
        }
        if (command != "open") {
            continue;
        }

        /* ===================== OPEN_CONNETION =================== */
        if (!(command_line >> ip >> port)) {
            continue;
        }
        request_type = OPEN_CONN_REQUEST;

        // 1. Socket
        sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in addr{};
        addr.sin_port = htons(port); 
        addr.sin_family = AF_INET;
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr); // 表示我们要连接到服务器的127.0.0.1:12323

        // 2. Try TCP connect
        printf("Try connect\n");
        if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
            perror("connect");
            close(sock);
            sock = -1;
            printf("TCP connection failed, idle\n");
            continue;
        }
        printf("TCP connection established\n");

        // 3. Request a connection
        established = open_conn_request();
        if (!established) {
            close(sock);
            sock = -1;
        }
    }

    /* ====================== Client Main Loop ===================== */
    while(established){
        printf("Client(%s:%hu)>", ip.c_str(), port);
        fflush(stdout);
        // Parse command
        /*
        MyFTP 的 Client 支持以下的命令
            open <IP> <port>: 建立一个到 <IP>:<port> 的连接
            ls: 获取对方当前工作目录下的文件列表。
            cd <directory>: 更改 Server 的工作目录，简单起见只需考虑<directory>为相对路径，若路径不存在则返回不存在。
            get <filename>: 将 Server 工作目录中的 <filename> 文件存放到 Client 工作目录的 <filename> 中。
            put <filename>: 将 Client 工作目录中的 <filename> 文件存放到 Server 工作目录的 <filename> 中。
            sha256 <filename>: 在 Server 工作目录中查询某个文件的 sha256 值，若不存在则返回不存在。
            quit: 如有连接则断开连接，回到 open 前的状态；如果已经是 open 前的状态，则关闭 Client
        */

        if (!getline(cin, line)) {
            break;
        }
        command_line.clear();
        command_line.str(line);
        command_line >> command;
        if (command == "ls") {
            request_type = LIST_REQUEST;
        } else if (command == "cd") {
            command_line >> argument;
            request_type = CHANGE_DIR_REQUEST;
        } else if (command == "get") {
            command_line >> argument;
            request_type = GET_REQUEST;
        } else if (command == "put") {
            command_line >> argument;
            request_type = PUT_REQUEST;
        } else if (command == "sha256") {
            command_line >> argument;
            request_type = SHA_REQUEST;
        } else if (command == "quit") {
            request_type = QUIT_REQUEST;
        } else {
            continue;
        }

        // Send a request
        struct Header h{};
        h.type = request_type;
        
        switch (request_type) {
            case LIST_REQUEST:
                list_request();
            case CHANGE_DIR_REQUEST:
                break;
            case GET_REQUEST:
                break;
            case PUT_REQUEST:
                break;
            case SHA_REQUEST:
                break;
            case QUIT_REQUEST:
                printf("Connection closed; Client idle.\n");
                established = false;
                break;
            default:
                break;
        }
        printf("Client finished a request\n");
    }

    // Close connection
    return 0;
}



/* ========================= Helpers ==========================*/

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
void list_request(){
    struct Header h{};
    h.type = LIST_REQUEST;
    h.length = htonl(12);
    send(sock, &h, sizeof(h), 0);
    printf("Client sent LIST_REQUEST\n");   
}
