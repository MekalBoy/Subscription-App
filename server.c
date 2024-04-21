#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "helpers.h"

#define MAX_CONNECTIONS 32
#define MAX_PAYLOAD 1501

// Primeste date de pe connfd1 si trimite mesajul receptionat pe connfd2
int receive_and_send(int connfd1, int connfd2, size_t len) {
  int bytes_received;
  char buffer[len];

  // Primim exact len octeti de la connfd1
  bytes_received = recv_all(connfd1, buffer, len);
  // S-a inchis conexiunea
  if (bytes_received == 0) {
    return 0;
  }
  DIE(bytes_received < 0, "recv");

  // Trimitem mesajul catre connfd2
  int rc = send_all(connfd2, buffer, len);
  if (rc <= 0) {
    perror("send_all");
    return -1;
  }

  return bytes_received;
}

void run_chat_server(int listenfd) {
  struct sockaddr_in client_addr1;
  struct sockaddr_in client_addr2;
  socklen_t clen1 = sizeof(client_addr1);
  socklen_t clen2 = sizeof(client_addr2);

  int connfd1 = -1;
  int connfd2 = -1;
  int rc;

  // Setam socket-ul listenfd pentru ascultare
  rc = listen(listenfd, 2);
  DIE(rc < 0, "listen");

  // Acceptam doua conexiuni
  printf("Astept conectarea primului client...\n");
  connfd1 = accept(listenfd, (struct sockaddr *)&client_addr1, &clen1);
  DIE(connfd1 < 0, "accept");

  printf("Astept connectarea clientului 2...\n");
  connfd2 = accept(listenfd, (struct sockaddr *)&client_addr2, &clen2);
  DIE(connfd2 < 0, "accept");

  while (1) {
    printf("Primesc de la 1 si trimit catre 2...\n");
    int rc = receive_and_send(connfd1, connfd2, sizeof(struct chat_packet));
    if (rc <= 0) {
      break;
    }

    printf("Primesc de la 2 si trimit catre 1...\n");
    rc = receive_and_send(connfd2, connfd1, sizeof(struct chat_packet));
    if (rc <= 0) {
      break;
    }
  }

  // Inchidem conexiunile si socketii creati
  close(connfd1);
  close(connfd2);
}

void run_chat_multi_server(int tcpfd, int udpfd) {
  struct pollfd poll_fds[MAX_CONNECTIONS];
  int num_sockets = 3; // stdin, tcp, udp to begin with
  int rc, nagle;

  char buf[1501];

  struct chat_packet received_packet;

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
    DIE(rc < 0, "Something about poll failing.");

    for (int i = 0; i < num_sockets; i++) {
      if (poll_fds[i].revents & POLLIN) {
        if (poll_fds[i].fd == STDIN_FILENO) { // Probably an 'exit' command
          fgets(buf, 5, stdin);

          if (strncmp(buf, "exit", sizeof("exit") - 1) == 0) {
            // TODO: disconnect everyone gracefully
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

          // Adaugam noul socket intors de accept() la multimea descriptorilor de citire
          poll_fds[num_sockets].fd = newsockfd;
          poll_fds[num_sockets].events = POLLIN;
          num_sockets++;

          printf("New client <id> connected from %s:%d\nsocket client %d\n",
                 inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port),
                 newsockfd);
        } else if (poll_fds[i].fd == udpfd) { // UDP (new content)
          // Am primit date pe unul din socketii de client, asa ca le receptionam
          rc = recv_all(poll_fds[i].fd, &received_packet,
                            sizeof(received_packet));
          DIE(rc < 0, "recv");

          if (rc == 0) {
            printf("Socket-ul client %d a inchis conexiunea\n", i);
            close(poll_fds[i].fd);

            // Scoatem din multimea de citire socketul inchis
            for (int j = i; j < num_sockets - 1; j++) {
              poll_fds[j] = poll_fds[j + 1];
            }

            num_sockets--;
          } else {
            printf("S-a primit de la clientul de pe socketul %d mesajul: %s\n",
                   poll_fds[i].fd, received_packet.message);
            /* TODO 2.1: Trimite mesajul catre toti ceilalti clienti */
          }
        } else { // TCP (clients)
          // Am primit date pe unul din socketii de client, asa ca le receptionam
          rc = recv_all(poll_fds[i].fd, &received_packet,
                            sizeof(received_packet));
          DIE(rc < 0, "recv");

          if (rc == 0) {
            printf("Socket-ul client %d a inchis conexiunea\n", i);
            close(poll_fds[i].fd);

            // Scoatem din multimea de citire socketul inchis
            for (int j = i; j < num_sockets - 1; j++) {
              poll_fds[j] = poll_fds[j + 1];
            }

            num_sockets--;
          } else {
            printf("S-a primit de la clientul de pe socketul %d mesajul: %s\n",
                   poll_fds[i].fd, received_packet.message);
            /* TODO 2.1: Trimite mesajul catre toti ceilalti clienti */
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
