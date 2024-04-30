#ifndef __COMMON_H__
#define __COMMON_H__

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>
#include <netdb.h>
#include <stddef.h>

int send_all(int sockfd, void *buff, size_t len);
int recv_all(int sockfd, void *buff, size_t len);
struct double_list *addTopic(struct double_list *head, char *topic);
struct double_list *deleteTopic(struct double_list *head, char *topic);

#define MSG_MAXSIZE 1500

// Message received by server from UDP clients
struct __attribute__((__packed__)) udp_msg {
  char topic[50];
  char type;
  char payload[MSG_MAXSIZE];
};

// Info message sent by server to TCP clients
// Using this, they know how many bytes of the payload to receive
struct tcp_msg {
  struct in_addr udp_ip;
  in_port_t udp_port;
  int datalen;
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
