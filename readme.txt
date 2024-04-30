
# Subscription App

This project implements a server and two types of clients: UDP (senders) and TCP (receivers). TCP clients can subscribe/unsubscribe from various topics by interacting with the server. Data is sent from the UDP clients to the server and is passed on from there to the TCP clients if they are subscribed to the appropriate topic.

## Design
The main files comprising the project are `server.c`, `subscriber.c`, `common.c`, `common.h` and `helpers.h`. The first two contain the server and TCP client code respectively, while the `common.c` contains functionality for sending/receiving buffer-limited byte streams as well as two large functions for adding/removing elements from a doubly linked list.\

The `helpers.h` file simply contains a DIE macro which helps kill the program when encountering undesired/undefined behaviour such as being unable to secure a TCP socket.\

The bread and butter of the structure is contained within `common.h`, with various structures which help with communication between the various elements of the project. These are described below.

## Structures

### udp_msg
```c
struct __attribute__((__packed__)) udp_msg {
  char topic[50];
  char type;
  char payload[MSG_MAXSIZE];
};
```
* The data packet sent by UDP clients to the server
* Contains the topic, type and payload (formatted to the appropriate type)

### tcp_msg
```c
struct tcp_msg {
  struct in_addr udp_ip;
  in_port_t udp_port;
  int datalen;
};
```
* Info message sent by server to TCP clients
* Using this, they know how many bytes of the payload to receive
* Clients also know what the source of the data is (via udp_ip and udp_port)

### tcp_sub
```c
struct tcp_sub {
  char type; // 0 - unsubscribe, 1 - subscribe
  uint16_t len; // topic length
  char topic[50];
};
```
* Request sent by TCP clients to server
* Type of request (subscribe/unsubscribe) dictated by the type char

### tcp_id
```c
struct tcp_id {
  char id[10];
};
```
* Info message sent by TCP clients to server when connecting
* Helps the server find the client in the database (if reconnecting)

### client_entry
```c
struct client_entry { // used in the client database
  char id[10]; // client's id
  int sockfd; // client's used sockfd
  struct double_list *topiclist; // client's list of topics
};
```
* Entry structure used in the server for the client database
* Contains a double linked list of topics for easy adding/removing

### double_list
```c
struct double_list {
  char topic[50];
  struct double_list *prev, *next;
};
```
* Double list containing a 50-character topic
* Used instead of an array due to always having to go through each one when trying to match a topic, so the issue is only when adding/removing a topic for a client
