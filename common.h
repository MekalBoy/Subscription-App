#ifndef __COMMON_H__
#define __COMMON_H__

#include <stddef.h>
#include <stdint.h>

int send_all(int sockfd, void *buff, size_t len);
int recv_all(int sockfd, void *buff, size_t len);
struct double_list *addTopic(struct double_list *head, char *topic);
struct double_list *deleteTopic(struct double_list *head, char *topic);

#define MSG_MAXSIZE 1500

struct data_type0 {
  unsigned char sign;
  uint32_t INT; // network byte order
};

struct data_type1 {
  uint16_t SHORT_REAL; // modulul numarului inmultit cu 100 (huh?)
};

struct data_type2 {
  unsigned char sign;
  uint32_t FLOAT; // network byte order (modululu numarului obtinut din alipirea partii intregi de partea zecimala a numarului)
  uint8_t power; // modulul puterii negative a lui 10 cu care trebuie inmultit modulul pentru a obtine numarul original (in modul)
};

struct data_type3 {
  unsigned char STRING[MSG_MAXSIZE];
};

struct udp_msg {
  char topic[50];
  char type;
  union udpData {
    struct data_type0 t0; // sign + INT
    struct data_type1 t1; // SHORT_REAL
    struct data_type2 t2; // sign + FLOAT
    struct data_type3 t3; // STRING[MSG_MAXSIZE]
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
  int sockfd; // client's used sockfd
  struct double_list *topiclist; // client's list of topics
};

struct double_list {
  char topic[50];
  struct double_list *prev, *next;
};

#endif
