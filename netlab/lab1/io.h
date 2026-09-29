/** Wrappers for TCP send() and recv() */

#pragma once

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <poll.h>
#include <sys/socket.h>

// Send message bits by batch_size=len, until all bytes in buf are sent
inline void send_all(int sock, const void *buf, size_t len) {
    size_t total_sent = 0;
    while (total_sent < len) {
        ssize_t sent = send(sock, (const char*)buf + total_sent, len - total_sent, 0);
        if (sent <= 0) {
            perror("send");
            exit(EXIT_FAILURE);
        }
        total_sent += sent;
    }
}



// Receive message bits by batch_size=len, until all bytes in buf are received
inline bool recv_all(int sock, void *buf, size_t len, int timeout_ms = -1) {
    size_t total_received = 0;
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeout_ms);

    while (total_received < len) {  // Blocking

        // 超时检查
        if (timeout_ms >= 0) {
            const auto remaining = std::chrono::duration_cast<
                std::chrono::milliseconds>(deadline -
                                           std::chrono::steady_clock::now());
            if (remaining.count() <= 0) {
                errno = ETIMEDOUT;
                return false;
            }
            pollfd descriptor{sock, POLLIN, 0};
            int ready;
            do {
                ready = poll(&descriptor, 1,
                             static_cast<int>(remaining.count()));
            } while (ready < 0 && errno == EINTR);
            if (ready == 0) {
                errno = ETIMEDOUT;
                return false;
            }
            if (ready < 0) {
                perror("poll");
                return false;
            }
        }


        ssize_t received = recv(sock, (char*)buf + total_received, len - total_received, 0);
        if (received < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("recv");
            return false; // 返回false,避免用exit()退出整个进程
        }

        if (received == 0) {
            return false;
        }
        total_received += received;
    }
    return true;
}
