#include "common.h"

#include <sys/socket.h>
#include <sys/types.h>

/*
    TODO 1.1: Rescrieți funcția de mai jos astfel încât ea să facă primirea
    a exact len octeți din buffer.
*/
int recv_all(int sockfd, void *buffer, size_t len) {
  size_t bytes_received = 0;
  size_t bytes_remaining = len;
  char *buff = buffer;
  /*
    while(bytes_remaining) {
      TODO: Make the magic happen
    }
  */

  /*
    TODO: Returnam exact cati octeti am citit
  */
  return recv(sockfd, buffer, len, 0);
}

/*
    TODO 1.2: Rescrieți funcția de mai jos astfel încât ea să facă trimiterea
    a exact len octeți din buffer.
*/

int send_all(int sockfd, void *buffer, size_t len) {
  size_t bytes_sent = 0;
  size_t bytes_remaining = len;
  char *buff = buffer;
  /*
    while(bytes_remaining) {
      TODO: Make the magic happen
    }
  */

  /*
    TODO: Returnam exact cati octeti am trimis
  */
  return send(sockfd, buffer, len, 0);
}
