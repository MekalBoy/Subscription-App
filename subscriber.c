#include "common.h"
#include "helpers.h"

void run_client(int tcpfd) {
  char buf[MSG_MAXSIZE + 1];
  memset(buf, 0, MSG_MAXSIZE + 1);

  struct tcp_sub sub_packet;
  struct tcp_msg data_packet;
  int payloadComing = 0;

  struct data_type0 t0;
  struct data_type1 t1;
  struct data_type2 t2;
  struct data_type3 t3;

  struct pollfd poll_fds[2];
  int num_sockets = 2; // stdin, tcp
  int rc;

  // Solely for 'exit'
  poll_fds[0].fd = STDIN_FILENO;
  poll_fds[0].events = POLLIN;

  poll_fds[1].fd = tcpfd;
  poll_fds[1].events = POLLIN;

  while (1) {
    rc = poll(poll_fds, num_sockets, -1);
    DIE(rc < 0, "Polling failed.");

    for (int i = 0; i < num_sockets; i++) {
      if (poll_fds[i].revents & POLLIN) {
        if (poll_fds[i].fd == STDIN_FILENO) { // exit, subscribe, unsubscribe
          memset(buf, 0, MSG_MAXSIZE + 1);

          fgets(buf, MSG_MAXSIZE, stdin);

          if (strncmp(buf, "exit", sizeof("exit") - 1) == 0) {
            for (int j = 0; j < num_sockets; j++) {
              close(poll_fds[j].fd);
            }
            return;
          } else if (strncmp(buf, "subscribe", sizeof("subscribe") - 1) == 0) {
            char topic[50];
            sscanf(buf, "%s %s\n", topic, topic);
            printf("Subscribed to topic %s\n", topic);

            sub_packet.type = 1;
            sub_packet.len = strlen(topic) + 1;
            strcpy(sub_packet.topic, topic);
            send(tcpfd, &sub_packet, sizeof(sub_packet), 0);

          } else if (strncmp(buf, "unsubscribe", sizeof("unsubscribe") - 1) == 0) {
            char topic[50];
            sscanf(buf, "%s %s\n", topic, topic);
            printf("Unsubscribed from topic %s\n", topic);

            sub_packet.type = 0;
            sub_packet.len = strlen(topic);
            strcpy(sub_packet.topic, topic);
            send(tcpfd, &sub_packet, sizeof(sub_packet), 0);
          }
        } else if (poll_fds[i].fd == tcpfd) { // server
          if (payloadComing) {
            switch (data_packet.type) {
              case 0: // uint32_t INT
                rc = recv_all(tcpfd, &t0, sizeof(t0));
                break;
              case 1: // uint16_t SHORT_REAL
                rc = recv_all(tcpfd, &t1, sizeof(t1));
                break;
              case 2: // float FLOAT
                rc = recv_all(tcpfd, &t2, sizeof(t2));
                break;
              case 3: // char STRING[MSG_MAXSIZE]
                rc = recv_all(tcpfd, &t3, sizeof(t3));
                break;
            }
          } else {
            rc = recv_all(tcpfd, &data_packet, sizeof(data_packet));
          }
          DIE(rc < 0, "Could not receive from server");

          if (rc == 0) { // server disconnected
            for (int j = 0; j < num_sockets; j++) {
              close(poll_fds[i].fd);
            }
            return;
          } else {
            if (payloadComing) {
              printf("%s:%d - %s - ", inet_ntoa(data_packet.udp_ip), ntohs(data_packet.udp_port), data_packet.topic);
              double SHORT_REAL;
              float FLOAT;

              switch (data_packet.type) {
                case 0: // uint32_t INT
                  printf("INT - %d\n", t0.INT * (t0.sign == 0 ? 1 : -1));
                  break;
                case 1: // uint16_t SHORT_REAL
                  SHORT_REAL = t1.SHORT_REAL;
                  printf("SHORT_REAL - %.2f\n", SHORT_REAL / 100);
                  break;
                case 2: // float FLOAT
                  FLOAT = (float)(t2.FLOAT) / (float)pow(10, t2.power) * (float)(t2.sign == 0 ? 1 : -1);
                  printf("FLOAT - %.*lf\n", t2.power, FLOAT);
                  break;
                case 3: // char STRING[MSG_MAXSIZE + 1]
                  printf("STRING - %s\n", t3.STRING);
                  break;
              }

              // go back to listening for info packets
              payloadComing = 0;
            } else {
              payloadComing = 1;
            }
          }
        }
      }
    }
  }
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

  // Disable buffer
  rc = setvbuf(stdout, NULL, _IONBF, BUFSIZ);
  DIE(rc < 0, "Could not disable buffer");

  // Obtinem un socket TCP pentru conectarea la server
  const int tcpfd = socket(AF_INET, SOCK_STREAM, 0);
  DIE(tcpfd < 0, "Could not obtain TCP socket");

  // Completăm in serv_addr adresa serverului, familia de adrese si portul pentru conectare
  struct sockaddr_in serv_addr;
  socklen_t socket_len = sizeof(struct sockaddr_in);

  memset(&serv_addr, 0, socket_len);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  rc = inet_pton(AF_INET, argv[2], &serv_addr.sin_addr.s_addr);
  DIE(rc <= 0, "inet_pton");

  DIE(strlen(argv[1]) > 10, "ID longer than 10");
  struct tcp_id client_id;
  strcpy(client_id.id, argv[1]);

  // Ne conectăm la server
  rc = connect(tcpfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  DIE(rc < 0, "Could not connect to server");

  send_all(tcpfd, &client_id, sizeof(client_id));

  run_client(tcpfd);

  // Inchidem conexiunea si socketul creat
  close(tcpfd);

  return 0;
}
