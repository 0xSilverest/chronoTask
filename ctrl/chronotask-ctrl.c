#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "socket.h"

void print_usage(const char *program_name) {
    printf("Usage: %s <command> [args]\n", program_name);
    printf("Commands:\n");
    printf("  pause              Pause the current task\n");
    printf("  resume             Resume the paused task\n");
    printf("  next               Skip to the next task\n");
    printf("  previous           Go back to the previous task\n");
    printf("  extend <minutes>   Extend the current task by specified minutes\n");
    printf("  status             Get the current status of ChronoTask\n");
    printf("  abort              Terminate the ChronoTask program\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    const char *command = argv[1];
    char full_command[BUFFER_SIZE];

    if (strcmp(command, "pause") == 0 || 
        strcmp(command, "resume") == 0 || 
        strcmp(command, "next") == 0 || 
        strcmp(command, "previous") == 0 ||
        strcmp(command, "status") == 0 ||
        strcmp(command, "abort") == 0) {
        strncpy(full_command, command, BUFFER_SIZE);
    } else if (strcmp(command, "extend") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: 'extend' command requires minutes argument\n");
            return 1;
        }
        snprintf(full_command, BUFFER_SIZE, "extend %s", argv[2]);
    } else {
        fprintf(stderr, "Error: Unknown command '%s'\n", command);
        print_usage(argv[0]);
        return 1;
    }

    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        return -1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(sock);
        return -1;
    }

    if (send_message(sock, full_command) == -1) {
        perror("send_message");
        close(sock);
        return -1;
    }

    char response[BUFFER_SIZE];
    if (receive_message(sock, response, BUFFER_SIZE) > 0) {
        printf("Response: %s\n", response);
    }

    close(sock);
    return 0;
}
