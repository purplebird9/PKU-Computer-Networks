/**
 * MyFTP Protocol
 *
 *   Byte
 *   offset   0              6    7    8             12
 *            +--------------+----+----+--------------+
 *            |   protocol   |type|stat|    length    |
 *            +--------------+----+----+--------------+
 *            |     6 B      |1 B |1 B |     4 B      |
 *
 *   Total: 12 bytes
 */


#pragma once
#include <cstdint>

// |802.3 Header|IP Header|TCP Header|myFTP Header|myFTP Data|

// MyFTP Protocol Header: 6+1+1+4=12B
struct __attribute((packed)) Header{
    /* protocol magic number (6 bytes) */
    unsigned char protocol[6] = {0xc1, 0xa1, 0x10, 'f', 't', 'p'};
    uint8_t type;    /* type (1 byte) */
    uint8_t status;  /* status (1 byte) */
    uint32_t length; /* length (4 bytes) in Big endian */
};

// Message type code
enum MessageType : uint8_t {
    OPEN_CONN_REQUEST  = 0xA1,
    OPEN_CONN_REPLY    = 0xA2,
    LIST_REQUEST       = 0xA3,
    LIST_REPLY         = 0xA4,
    CHANGE_DIR_REQUEST = 0xA5,
    CHANGE_DIR_REPLY   = 0xA6,
    GET_REQUEST        = 0xA7,
    GET_REPLY          = 0xA8,
    FILE_DATA          = 0xFF,
    PUT_REQUEST        = 0xA9,
    PUT_REPLY          = 0xAA,
    SHA_REQUEST        = 0xAB,
    SHA_REPLY          = 0xAC,
    QUIT_REQUEST       = 0xAD,
    QUIT_REPLY         = 0xAE,
};



