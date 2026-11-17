#ifndef UTIL_H
#define UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rtp.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

uint32_t compute_checksum(const void *pkt, size_t n_bytes);

// Use it to initialize a complete RTP packet.
int rtp_makepkt(rtp_packet_t *pkt, uint32_t seq_num, uint8_t flags,
                const void *payload, size_t length);

// Use it to send a complete RTP packet(with checksum computed).
ssize_t rtp_sendto(int sockfd, const rtp_packet_t *pkt,
                   const struct sockaddr *dest_addr, socklen_t addrlen); // addrlen=16(ipv4) or 28(ipv6)

// Use it to receive and validate a complete RTP packet;
// A malformed packet fails with errno set to EBADMSG.
ssize_t rtp_recvfrom(int sockfd, rtp_packet_t *pkt,
                     struct sockaddr *src_addr, socklen_t *addrlen);

// Use it to display a help message
#define LOG_MSG(...)                                                    \
    do {                                                                \
        fprintf(stdout, "\033[40;32m[ INFO     ] \033[0m" __VA_ARGS__); \
        fflush(stdout);                                                 \
    } while (0)

// Use it to display debug information. Turn it on/off in CMakeLists.txt
#ifdef LDEBUG
#define LOG_DEBUG(...)                                                  \
    do {                                                                \
        fprintf(stderr, "\033[40;33m[ DEBUG    ] \033[0m" __VA_ARGS__); \
        fflush(stderr);                                                 \
    } while (0)
#else
#define LOG_DEBUG(...)
#endif

// Use it when an unrecoverable error happened
#define LOG_FATAL(...)                                                  \
    do {                                                                \
        fprintf(stderr, "\033[40;31m[ FATAL    ] \033[0m" __VA_ARGS__); \
        fflush(stderr);                                                 \
        exit(1);                                                        \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif
