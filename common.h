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
  char topic[50];
  char type;
  union udpData {
    uint32_t INT; // type 0
    uint16_t SHORT_REAL; // type 1
    float FLOAT; // type 2
    char STRING[MSG_MAXSIZE]; // type 3
  } payload;
};

struct tcp_sub {
  char type; // 0 - unsubscribe, 1 - subscribe
  uint16_t len; // topic length
  char topic[50];
};

struct tcp_id {
  char id[10];
};

struct client_entry { // used in the client database
  char id[10]; // client's id
  char *topics[50]; // client's subscribed topics
};

#endif
