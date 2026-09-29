/** Wrappers for TCP send() and recv() */

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
inline void recv_all(int sock, void *buf, size_t len) {
    size_t total_received = 0;
    while (total_received < len) {
        ssize_t received = recv(sock, (char*)buf + total_received, len - total_received, 0);
        if (received <= 0) {
            perror("recv");
            exit(EXIT_FAILURE);
        }
        total_received += received;
    }
}
