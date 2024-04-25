/*
 * Protocoale de comunicatii
 * Laborator 7 - TCP si mulplixare
 * client.c
 */

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "helpers.h"

void run_client(int tcpfd) {
  char buf[MSG_MAXSIZE + 1];
  memset(buf, 0, MSG_MAXSIZE + 1);

  struct chat_packet sent_packet;
  struct chat_packet recv_packet;

  struct tcp_sub sub_packet;

  struct pollfd poll_fds[2];
  int num_sockets = 2; // stdin, tcp
  int rc;

  // Solely for 'exit'
  poll_fds[0].fd = STDIN_FILENO;
  poll_fds[0].events = POLLIN;

  poll_fds[1].fd = tcpfd;
  poll_fds[1].events = POLLIN;

  /*
    TODO 2.2: Multiplexati intre citirea de la tastatura si primirea unui
    mesaj, ca sa nu mai fie impusa ordinea.

    Hint: server::run_multi_chat_server
  */
  while (1) {
    rc = poll(poll_fds, num_sockets, -1);
    DIE(rc < 0, "Something about poll failing.");

    for (int i = 0; i < num_sockets; i++) {
      if (poll_fds[i].revents & POLLIN) {
        if (poll_fds[i].fd == STDIN_FILENO) { // exit, subscribe, unsubscribe
          memset(buf, 0, MSG_MAXSIZE + 1);

          fgets(buf, MSG_MAXSIZE, stdin);

          if (strncmp(buf, "exit", sizeof("exit") - 1) == 0) {
            // TODO: disconnect gracefully
            // send_all(tcpfd, NULL, 0);
            return;
          } else if (strncmp(buf, "subscribe", sizeof("subscribe") - 1) == 0) {
            char topic[50];
            sscanf(buf, "%s %s", topic, topic);
            // printf("%s\n", topic);

            sub_packet.type = 1;
            sub_packet.len = strlen(topic) + 1;
            strcpy(sub_packet.topic, topic);
            send_all(tcpfd, &sub_packet, sizeof(sub_packet));

            printf("Subscribed to topic %s.\n", topic);
          } else if (strncmp(buf, "unsubscribe", sizeof("unsubscribe") - 1) == 0) {
            char topic[50];
            sscanf(buf, "%s %s", topic, topic);

            sub_packet.type = 0;
            sub_packet.len = strlen(topic);
            strcpy(sub_packet.topic, topic);
            send_all(tcpfd, &sub_packet, sizeof(uint16_t) + sizeof(char) + sub_packet.len);

            printf("Unsubscribed from topic %s.\n", topic);
          }
        } else if (poll_fds[i].fd == tcpfd) { // server

        }
      }
    }
  }
  // while (fgets(buf, sizeof(buf), stdin) && !isspace(buf[0])) {
  //   sent_packet.len = strlen(buf) + 1;
  //   strcpy(sent_packet.message, buf);

  //   // Trimitem pachetul la server.
  //   send_all(tcpfd, &sent_packet, sizeof(sent_packet));

  //   // Primim un mesaj de la server si il afisam.
  //   int rc = recv_all(tcpfd, &recv_packet, sizeof(recv_packet));
  //   if (rc <= 0) {
  //     break;
  //   }

  //   printf("%s\n", recv_packet.message);
  // }
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    printf("\n Usage: %s <id> <ip> <port>\n", argv[0]);
    return 1;
  }

  // Parsam port-ul ca un numar
  uint16_t port;
  int rc = sscanf(argv[3], "%hu", &port);
  DIE(rc != 1, "Given port is invalid");

  // Obtinem un socket TCP pentru conectarea la server
  const int tcpfd = socket(AF_INET, SOCK_STREAM, 0);
  DIE(tcpfd < 0, "Could not obtain TCP socket.");

  // Completăm in serv_addr adresa serverului, familia de adrese si portul pentru conectare
  struct sockaddr_in serv_addr;
  socklen_t socket_len = sizeof(struct sockaddr_in);

  memset(&serv_addr, 0, socket_len);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  rc = inet_pton(AF_INET, argv[2], &serv_addr.sin_addr.s_addr);
  DIE(rc <= 0, "inet_pton");

  DIE(strlen(argv[1]) > 10, "ID longer than 10.");
  struct tcp_id client_id;
  strcpy(client_id.id, argv[1]);

  // Ne conectăm la server
  rc = connect(tcpfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  DIE(rc < 0, "Could not connect to server.");

  send_all(tcpfd, &client_id, sizeof(client_id));

  run_client(tcpfd);

  // Inchidem conexiunea si socketul creat
  close(tcpfd);

  return 0;
}
