#include <arpa/inet.h>
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>
#include "server_replies.h"

#include "protocol.h"
#include "io.h"

using namespace std;
namespace fs = std::filesystem;

static string shell_quote(const string& value)
{
    string result = "'";
    for (char ch : value) {
        if (ch == '\'') {
            result += "'\\''";
        } else {
            result += ch;
        }
    }
    result += "'";
    return result;
}


void open_conn_reply(int sock){
    struct Header hs{};
    hs.type = OPEN_CONN_REPLY;
    hs.status = 1;
    hs.length = htonl(12);
    send_all(sock, &hs, sizeof(hs));
    printf("Server sent OPEN_CONN_REPLY\n");
}

void list_reply(int client_sock, const fs::path& cwd)
{
    vector<string> filenames;
    // 因为一个线程一个cwd，但是popen列出整个server进程的真实目录
    // 所以不能用popen
    for (const auto& entry : fs::directory_iterator(cwd)) {
        if (entry.is_regular_file()) {
            filenames.push_back(entry.path().filename().string());
        }
    }
    sort(filenames.begin(), filenames.end());

    string result;
    for (const string& filename : filenames) {
        result += filename + "\n";
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


void get_reply(int sock, const fs::path& cwd)
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

    fs::path file_path = cwd / filename;
    FILE* file = fopen(file_path.c_str(), "rb"); // rb: read binary,只读

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


void put_reply(int sock, const fs::path& cwd)
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
    fs::path file_path = cwd / filename;
    FILE* file = fopen(file_path.c_str(), "wb"); // wb: write binary，不存在就创建、覆盖写
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


void change_dir_reply(int sock, fs::path& cwd)
{
    // Receive directory name payload
    string dir;
    char ch;
    do {
        if (!recv_all(sock, &ch, sizeof(ch))) {
            return;
        }
        if (ch != '\0') {
            dir += ch;
        }
    } while (ch != '\0');

    // Attempt to change directory
    // 仅对于这个特定client，其它client的工作目录不能影响

    fs::path requested_dir(dir);
    fs::path new_dir;
    error_code ec;
    if (!requested_dir.is_absolute()) {// 如果是相对路径，则基于当前工作目录
        new_dir = fs::canonical(cwd / requested_dir, ec); //canonical:返回绝对路径
    }
    else{
        new_dir = fs::canonical(requested_dir, ec);
    }
    bool success = !ec && fs::is_directory(new_dir, ec);
    if (success) {
        cwd = new_dir;
    }

    // Reply
    struct Header hs{};
    hs.type = CHANGE_DIR_REPLY;
    hs.status = success ? 1 : 0; // 1 for success, 0 for failure
    hs.length = htonl(12);
    send_all(sock, &hs, sizeof(hs));
    printf("Server sent CHANGE_DIR_REPLY for directory: %s with status: %d\n", dir.c_str(), hs.status);

}


void sha_reply(int sock, const fs::path& cwd)
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

    fs::path file_path = cwd / filename;
    FILE* file = fopen(file_path.c_str(), "rb"); // rb: read binary,只读

    Header reply{};
    reply.type = SHA_REPLY;
    reply.status = 0;
    reply.length = htonl(sizeof(Header));

    // file does not exist, send SHA_REPLY with status 0
    if (file == nullptr) {
        send_all(sock, &reply, sizeof(reply));
        printf("Server sent SHA_REPLY: file not found\n");
        return;
    }
    fclose(file);

    // Calculate SHA256 hash with popen
    string command = "sha256sum -- " + shell_quote(file_path.string());
    FILE* pipe = popen(command.c_str(), "r");
    if (pipe == nullptr) {
        perror("popen");
        send_all(sock, &reply, sizeof(reply));
        return;
    }

    char hash[65]{};
    const size_t hash_length = fread(hash, 1, 64, pipe);
    const int command_status = pclose(pipe);
    if (hash_length != 64 || command_status != 0) {
        fprintf(stderr, "Failed to calculate SHA256\n");
        send_all(sock, &reply, sizeof(reply));
        return;
    }

    // file exists, send SHA_REPLY with status 1
    reply.status = 1;
    send_all(sock, &reply, sizeof(reply));

    // send FILE_DATA header
    struct Header hs_data{};
    hs_data.type = FILE_DATA;
    hs_data.status = 0;
    hs_data.length = htonl(sizeof(Header) + sizeof(hash));
    send_all(sock, &hs_data, sizeof(hs_data));
    send_all(sock, hash, sizeof(hash));

    printf("Server sent SHA_REPLY and FILE_DATA\n");
}
