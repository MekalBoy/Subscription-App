#ifndef __COMMON_H__
#define __COMMON_H__

#include <stddef.h>
#include <stdint.h>

int send_all(int sockfd, void *buff, size_t len);
int recv_all(int sockfd, void *buff, size_t len);

#define MSG_MAXSIZE 1500

struct chat_packet {
  uint16_t len;
  char message[MSG_MAXSIZE];
};

struct udp_msg {
  union udpData {
    uint32_t INT; // type 0
    uint16_t SHORT_REAL; // type 1
    float FLOAT; // type 2
    char STRING[MSG_MAXSIZE]; // type 3
  } payload;
  char type;
};

#endif
