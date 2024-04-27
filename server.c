#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "helpers.h"

#define MAX_CONNECTIONS 32
#define MAX_PAYLOAD 50 + 1 + 1500

struct client_entry client_db[MAX_CONNECTIONS];
int client_db_len = 0;

// Returns a client entry if the client is found, or NULL
struct client_entry *findClientByID(char id[10]) {
  for (int i = 0; i < client_db_len; i++) {
    if (strcmp(client_db[i].id, id) == 0) {
      return &client_db[i];
    }
  }

  return NULL;
}

// Returns a client entry if the client is found, or NULL
struct client_entry *findClientByFD(int fd) {
  for (int i = 0; i < client_db_len; i++) {
    if (client_db[i].sockfd == fd) {
      return &client_db[i];
    }
  }

  return NULL;
}

// Returns 0 on success, -1 if the client's ID is already present.
int addClient(char id[10], int sockfd) {
  struct client_entry *prevClient = findClientByID(id);
  if (prevClient != NULL) {
    if (prevClient->sockfd == -1) { // client was here previously, reconnect them
      prevClient->sockfd = sockfd;
      return 1;
    }
    return -1; // there's already an active client with that ID
  }

  struct client_entry newClient;
  strcpy(newClient.id, id);
  newClient.sockfd = sockfd;
  newClient.topiclist = NULL;

  client_db[client_db_len++] = newClient;
  return 0;
}

void subscribeTopic(struct client_entry *entry, char *topic) {
  entry->topiclist = addTopic(entry->topiclist, topic);
}

void unsubscribeTopic(struct client_entry *entry, char *topic) {
  entry->topiclist = deleteTopic(entry->topiclist, topic);
}

int findTopic (struct double_list *head, char *topic) {
  struct double_list *curr = head;
  while (curr != NULL) {
    if (strcmp(curr->topic, topic) == 0) {
      return 0;
    }

    curr = curr->next;
  }

  return -1;
}

void run_chat_multi_server(int tcpfd, int udpfd) {
  struct pollfd poll_fds[MAX_CONNECTIONS];
  int num_sockets = 3; // stdin, tcp, udp to begin with
  int rc, nagle;

  char buf[MAX_PAYLOAD];

  struct udp_msg udp_packet;

  struct tcp_id client_id;
  struct tcp_sub client_sub;

  // Listen for TCP subscribers
  rc = listen(tcpfd, MAX_CONNECTIONS);
  DIE(rc < 0, "Could not listen for TCP.");

  // Solely for 'exit'
  poll_fds[0].fd = STDIN_FILENO;
  poll_fds[0].events = POLLIN;

  poll_fds[1].fd = tcpfd;
  poll_fds[1].events = POLLIN;

  poll_fds[2].fd = udpfd;
  poll_fds[2].events = POLLIN;

  while (1) {
    rc = poll(poll_fds, num_sockets, -1);
    DIE(rc < 0, "Polling failed.");

    for (int i = 0; i < num_sockets; i++) {
      if (poll_fds[i].revents & POLLIN) {
        if (poll_fds[i].fd == STDIN_FILENO) { // Probably an 'exit' command
          fgets(buf, 5, stdin);

          if (strncmp(buf, "exit", sizeof("exit") - 1) == 0) {
            // TODO: disconnect everyone gracefully
            for (int j = 0; j < num_sockets; j++) {
              close(poll_fds[i].fd);
            }
            return;
          }
        } else if (poll_fds[i].fd == tcpfd) { // TCP (new connection)
          struct sockaddr_in cli_addr;
          socklen_t cli_len = sizeof(cli_addr);
          const int newsockfd = accept(tcpfd, (struct sockaddr *)&cli_addr, &cli_len);
          DIE(newsockfd < 0, "Could not accept TCP connection.");

          // Disable Nagle (TCP)
          rc = setsockopt(tcpfd, IPPROTO_TCP, TCP_NODELAY, &nagle, sizeof(nagle));
          DIE(rc < 0, "Could not disable Nagle at client.");

          rc = recv_all(newsockfd, &client_id, sizeof(client_id));
          DIE(rc < 0, "Could not receive client id.");

          rc = addClient(client_id.id, newsockfd);
          if (rc < 0) {
            close(newsockfd);
            printf("Client %s already connected.\n", client_id.id);
            continue;
          }

          // Add to poll only after it's confirmed good
          poll_fds[num_sockets].fd = newsockfd;
          poll_fds[num_sockets].events = POLLIN;
          num_sockets++;

          printf("New client %s connected from %s:%d.\n",
                  client_id.id, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

        } else if (poll_fds[i].fd == udpfd) { // UDP (new content)
          // Am primit date pe unul din socketii de client, asa ca le receptionam
          struct sockaddr_in cli_addr;
          socklen_t cli_len = sizeof(cli_addr);

          rc = recvfrom(poll_fds[i].fd, &udp_packet, sizeof(struct udp_msg), 0, (struct sockaddr *)&cli_addr, &cli_len);
          DIE(rc < 0, "Failed to recv from UDP.");

          if (rc == 0) {
            printf("Socket-ul client %d a inchis conexiunea\n", i);
            close(poll_fds[i].fd);

            // Scoatem din multimea de citire socketul inchis
            for (int j = i; j < num_sockets - 1; j++) {
              poll_fds[j] = poll_fds[j + 1];
            }

            num_sockets--;
          } else {
            printf("%s:%d - %s - ", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), udp_packet.topic);
            uint32_t uINT;
            int INT;
            double SHORT_REAL;

            switch (udp_packet.type) {
              case 0: // uint32_t INT
                uINT = ntohl(*((uint32_t*)(udp_packet.payload.t3.STRING)));
                INT = uINT * (udp_packet.payload.t3.STRING[-1] == 0 ? 1 : -1);
                printf("INT - %d\n", INT);
                break;
              case 1: // uint16_t SHORT_REAL
                SHORT_REAL = ntohs(*(uint16_t*)(udp_packet.payload.t3.STRING));
                printf("SHORT_REAL - %.2f\n", SHORT_REAL);
                break;
              case 2: // float FLOAT
                printf("FLOAT - %u\n", udp_packet.payload.t2.FLOAT * (udp_packet.payload.t2.sign == 0 ? 1 : -1));
                break;
              case 3: // char STRING[MSG_MAXSIZE]
                printf("STRING - %s\n", (char*)(udp_packet.payload.t3.STRING - 1));
                break;
            }
            /* TODO 2.1: Trimite mesajul catre toti ceilalti clienti */
          }
        } else { // TCP (clients)
          struct client_entry *clientinfo = findClientByFD(poll_fds[i].fd);

          // Am primit date pe unul din socketii de client, asa ca le receptionam
          rc = recv_all(poll_fds[i].fd, &client_sub, sizeof(client_sub));
          DIE(rc < 0, "Failed to recv from TCP client.");

          if (rc == 0) {
            printf("Client %s disconnected.\n", clientinfo->id);
            // printf("Socket-ul client %d a inchis conexiunea\n", i);
            close(poll_fds[i].fd);
            clientinfo->sockfd = -1; // set sockfd inactive

            // Scoatem din multimea de citire socketul inchis
            for (int j = i; j < num_sockets - 1; j++) {
              poll_fds[j] = poll_fds[j + 1];
            }

            num_sockets--;
          } else {
            char topic[50];
            strncpy(topic, client_sub.topic, client_sub.len);

            if (client_sub.type == 0) { // 0 - unsubscribe from topic
              // rc = findTopic(clientinfo->topiclist, topic);
              // printf("Found topic: %d", rc);
              
              unsubscribeTopic(clientinfo, topic);

              // rc = findTopic(clientinfo->topiclist, topic);
              // printf("Found topic: %d", rc);
            } else if (client_sub.type == 1) {
              subscribeTopic(clientinfo, topic);
            }
            
          }
        }
      }
    }
  }
}


int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("\n Usage: %s <port>\n", argv[0]);
    return 1;
  }

  uint16_t port;
  int rc, tcpfd, udpfd;
  int nagle;

  // Parse port
  rc = sscanf(argv[1], "%hu", &port);
  DIE(rc != 1, "Given port is invalid.");

  // Disable buffer
  rc = setvbuf(stdout, NULL, _IONBF, BUFSIZ);
  DIE(rc < 0, "Could not disable buffer.");

  // Create TCP socket
  tcpfd = socket(AF_INET, SOCK_STREAM, 0);
  DIE(tcpfd < 0, "Could not get TCP socket.");

  // Create UDP socket
  udpfd = socket(AF_INET, SOCK_DGRAM, 0);
  DIE(tcpfd < 0, "Could not get UDP socket.");

  // Disable Nagle (TCP)
  rc = setsockopt(tcpfd, IPPROTO_TCP, TCP_NODELAY, &nagle, sizeof(nagle));
  DIE(rc < 0, "Could not disable Nagle at startup.");

  // Completăm in serv_addr adresa serverului, familia de adrese si portul
  // pentru conectare
  struct sockaddr_in serv_addr;
  socklen_t socket_len = sizeof(struct sockaddr_in);

  memset(&serv_addr, 0, socket_len);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
//   rc = inet_pton(AF_INET, argv[1], &serv_addr.sin_addr.s_addr);
//   DIE(rc <= 0, "inet_pton");

  // Bind TCP
  rc = bind(tcpfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
  DIE(rc < 0, "Could not bind TCP.");

  // Bind UDP
  rc = bind(udpfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
  DIE(rc < 0, "Could not bind UDP.");

  run_chat_multi_server(tcpfd, udpfd);

  // Inchidem fd-urile
  // close(listenfd);
  close(tcpfd);
  close(udpfd);

  return 0;
}
