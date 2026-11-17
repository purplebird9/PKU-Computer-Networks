#include "util.h"

#include <errno.h>
#include <string.h>

static uint32_t crc32_for_byte(uint32_t r) {
    for (int j = 0; j < 8; ++j) r = (r & 1 ? 0 : (uint32_t)0xEDB88320L) ^ r >> 1;
    return r ^ (uint32_t)0xFF000000L;
}

static void crc32(const void* data, size_t n_bytes, uint32_t* crc) {
    static uint32_t table[0x100];
    if (!*table)
        for (size_t i = 0; i < 0x100; ++i) table[i] = crc32_for_byte(i);
    for (size_t i = 0; i < n_bytes; ++i)
        *crc = table[(uint8_t)*crc ^ ((uint8_t*)data)[i]] ^ *crc >> 8;
}

// Computes checksum for `n_bytes` of data
//
// Hint 1: Before computing the checksum, you should set everything up
// and set the "checksum" field to 0. And when checking if a packet
// has the correct check sum, don't forget to set the "checksum" field
// back to 0 before invoking this function.
//
// Hint 2: `len + sizeof(rtp_header_t)` is the real length of a rtp
// data packet.
uint32_t compute_checksum(const void* pkt, size_t n_bytes) {
    uint32_t crc = 0;
    crc32(pkt, n_bytes, &crc);
    return crc;
}

int rtp_makepkt(rtp_packet_t* pkt, uint32_t seq_num, uint8_t flags,
                const void* payload, size_t length) {
    const uint8_t valid_flags = RTP_SYN | RTP_ACK | RTP_FIN;
    if (pkt == NULL || seq_num >= (1U << 30) ||
        (flags & (uint8_t)~valid_flags) != 0 ||
        ((flags & RTP_SYN) != 0 && (flags & RTP_FIN) != 0) ||
        (flags != 0 && length != 0) ||
        (payload == NULL && length != 0)) {
        errno = EINVAL;
        return -1;
    }
    if (length > PAYLOAD_MAX) {
        errno = EMSGSIZE;
        return -1;
    }

    memset(pkt, 0, sizeof(*pkt));
    pkt->rtp.seq_num = seq_num;
    pkt->rtp.length = (uint16_t)length;
    pkt->rtp.flags = flags;
    if (length != 0) memcpy(pkt->payload, payload, length);

    size_t pkt_size = sizeof(rtp_header_t) + length;
    pkt->rtp.checksum = compute_checksum(pkt, pkt_size);
    return 0;
}

ssize_t rtp_sendto(int sockfd, const rtp_packet_t* pkt,
                   const struct sockaddr* dest_addr, socklen_t addrlen) {
    if (pkt == NULL) {
        errno = EINVAL;
        return -1;
    }
    if (pkt->rtp.length > PAYLOAD_MAX) {
        errno = EMSGSIZE;
        return -1;
    }

    size_t pkt_size = sizeof(rtp_header_t) + pkt->rtp.length;
    rtp_packet_t wire_pkt; // 副本，原始包不能修改。
    memcpy(&wire_pkt, pkt, pkt_size);
    wire_pkt.rtp.checksum = 0;
    wire_pkt.rtp.checksum = compute_checksum(&wire_pkt, pkt_size);

    ssize_t sent;
    do {
        sent = sendto(sockfd, &wire_pkt, pkt_size, 0, dest_addr, addrlen);
    } while (sent < 0 && errno == EINTR);

    if (sent >= 0 && (size_t)sent != pkt_size) {
        errno = EIO;
        return -1;
    }
    return sent;
}

ssize_t rtp_recvfrom(int sockfd, rtp_packet_t* pkt, struct sockaddr* src_addr,
                     socklen_t* addrlen) {
    if (pkt == NULL) {
        errno = EINVAL;
        return -1;
    }

    rtp_packet_t wire_pkt;
    ssize_t received;
    do {
        received = recvfrom(sockfd, &wire_pkt, sizeof(wire_pkt), MSG_TRUNC,
                            src_addr, addrlen);
    } while (received < 0 && errno == EINTR);

    if (received < 0) return -1;
    if ((size_t)received < sizeof(rtp_header_t) ||
        (size_t)received > sizeof(wire_pkt) ||
        wire_pkt.rtp.length > PAYLOAD_MAX ||
        (size_t)received != sizeof(rtp_header_t) + wire_pkt.rtp.length) {
        errno = EBADMSG;
        return -1;
    }

    uint32_t received_checksum = wire_pkt.rtp.checksum;
    wire_pkt.rtp.checksum = 0;
    if (compute_checksum(&wire_pkt, (size_t)received) != received_checksum) {
        errno = EBADMSG;
        return -1;
    }
    wire_pkt.rtp.checksum = received_checksum;
    memcpy(pkt, &wire_pkt, (size_t)received);
    return received;
}
