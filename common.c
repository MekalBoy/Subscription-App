#include "common.h"

int recv_all(int sockfd, void *buffer, size_t len) {
  size_t bytes_received = 0;
  char *buff = buffer;

  while (bytes_received < len) {
    int bytes = recv(sockfd, buff + bytes_received, len - bytes_received, 0);

    if (bytes <= 0) {
      if (bytes == 0) // connection closed
        break;
      else // error
        return bytes;
    }

    bytes_received += bytes;
  }

  return bytes_received;
}

int send_all(int sockfd, void *buffer, size_t len) {
  size_t bytes_sent = 0;
  char *buff = buffer;

  while (bytes_sent < len) {
    int bytes = send(sockfd, buff + bytes_sent, len - bytes_sent, 0);

    if (bytes <= 0) {
      return bytes; // error
    }

    bytes_sent += bytes;
  }

  return bytes_sent;
}

// Returns the head of the topic_list with the topic inserted
struct double_list *addTopic(struct double_list *head, char *topic) {
  if (head == NULL) {
    head = malloc(sizeof(struct double_list));
    head->next = NULL;
    head->prev = NULL;
    strcpy(head->topic, topic);

    return head;
  }

  struct double_list *curr = head;

  // topic at the start already
  if (strcmp(curr->topic, topic) == 0) {
    return head;
  }

  // topic somewhere in the middle already
  while (curr->next != NULL) {
    if (strcmp(curr->topic, topic) == 0) {
      return head;
    }

    curr = curr->next;
  }

  // topic at the end already
  if (strcmp(curr->topic, topic) == 0) {
    return head;
  }

  struct double_list *newTopic = malloc(sizeof(struct double_list));
  curr->next = newTopic;
  newTopic->prev = curr;
  newTopic->next = NULL;
  strcpy(newTopic->topic, topic);

  return head;
}

// Returns the head of the topic_list with the topic deleted
struct double_list *deleteTopic(struct double_list *head, char *topic) {
  if (head == NULL) {
    return NULL; // no list, no problem
  }

  struct double_list *curr;

  // topic at the start
  if (strcmp(head->topic, topic) == 0) {
    if (head->next == NULL) { // no other topic
      free(head);

      return NULL;
    } else { // replace head
      curr = head->next; // curr is new head
      curr->prev = NULL;
      free(head);

      return curr;
    }
  }

  curr = head->next;
  while (curr != NULL) {
    if (strcmp(curr->topic, topic) == 0) { // curr needs to be removed
      if (curr->next == NULL) { // curr is last
        curr->prev->next = NULL;
        free(curr);

        return head;
      } else { // curr is somewhere in the middle
        curr->prev->next = curr->next;
        curr->next->prev = curr->prev;
        free(curr);

        return head;
      }
    }

    curr = curr->next;
  }

  // topic does not exist so we return the list unaffected
  return head;
}
