#ifndef SOCKET_H
#define SOCKET_H

#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/chronotask.sock"
#define BUFFER_SIZE 1024

int create_socket(void);
int accept_connection(int server_socket);
int send_message(int socket, const char *message);
int receive_message(int socket, char *buffer, int buffer_size);

#endif
