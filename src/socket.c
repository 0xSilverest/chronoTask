#include "socket.h"
#include "error_report.h"
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

int create_socket(void) {
    int server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_socket == -1) {
        LOG_ERROR("Failed to create socket: %s", strerror(errno));
        return -1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    unlink(SOCKET_PATH);
    if (bind(server_socket, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        LOG_ERROR("Failed to bind socket: %s", strerror(errno));
        close(server_socket);
        return -1;
    }

    if (listen(server_socket, 5) == -1) {
        LOG_ERROR("Failed to listen on socket: %s", strerror(errno));
        close(server_socket);
        return -1;
    }

    LOG_INFO("Socket created and listening on %s", SOCKET_PATH);
    return server_socket;
}

int accept_connection(int server_socket) {
    int client_socket = accept(server_socket, NULL, NULL);
    if (client_socket == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            LOG_ERROR("Accept failed: %s", strerror(errno));
        }
        return -1;
    }
    LOG_DEBUG("Accepted new connection");
    return client_socket;
}

int send_message(int socket, const char *message) {
    ssize_t bytes_sent = send(socket, message, strlen(message), 0);
    if (bytes_sent == -1) {
        LOG_ERROR("Send failed: %s", strerror(errno));
        return -1;
    }
    LOG_DEBUG("Sent %zd bytes", bytes_sent);
    return bytes_sent;
}

int receive_message(int socket, char *buffer, int buffer_size) {
    ssize_t bytes_received = recv(socket, buffer, buffer_size - 1, 0);
    if (bytes_received == -1) {
        LOG_ERROR("Receive failed: %s", strerror(errno));
        return -1;
    }
    buffer[bytes_received] = '\0';
    LOG_DEBUG("Received %zd bytes", bytes_received);
    return bytes_received;
}
