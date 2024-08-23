#include "chronotask.h"
#include "config.h"
#include "task.h"
#include "window.h"
#include "overlay.h"
#include "audio.h"
#include "socket.h"
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>

volatile sig_atomic_t keep_running = 1;
int paused = 0;
time_t pause_start_time = 0;
time_t total_pause_duration = 0;

void handle_sigint(int sig) {
    (void)sig;
    keep_running = 0;
}

void handle_command(int client_socket, const char* cmd) {
    char response[BUFFER_SIZE];

    if (strcmp(cmd, "pause") == 0) {
        if (!paused) {
            paused = 1;
            pause_start_time = time(NULL);
            strcpy(response, "Task paused");
        } else {
            strcpy(response, "Task already paused");
        }
    } else if (strcmp(cmd, "resume") == 0) {
        if (paused) {
            paused = 0;
            total_pause_duration += difftime(time(NULL), pause_start_time);
            pause_start_time = 0;
            strcpy(response, "Task resumed");
        } else {
            strcpy(response, "Task already running");
        }
    } else if (strcmp(cmd, "next") == 0) {
        move_to_next_task();
        sprintf(response, "Moved to next task: %s", get_current_task_name());
    } else if (strcmp(cmd, "previous") == 0) {
        move_to_previous_task();
        sprintf(response, "Moved to previous task: %s", get_current_task_name());
    } else if (strncmp(cmd, "extend ", 7) == 0) {
        int minutes = atoi(cmd + 7);
        extend_current_task(minutes * 60);
        sprintf(response, "Extended task by %d minutes", minutes);
    } else if (strcmp(cmd, "status") == 0) {
        time_t now = time(NULL);
        int elapsed = difftime(now, get_task_start_time()) - total_pause_duration;
        int remaining = get_current_task_duration() - elapsed;
        sprintf(response, "Current task: %s, Time remaining: %d seconds, Status: %s", 
                get_current_task_name(), remaining, paused ? "Paused" : "Running");
    } else if (strcmp(cmd, "abort") == 0) {
        strcpy(response, "Terminating ChronoTask");
        send_message(client_socket, response);
        keep_running = 0;
        return;
    } else {
        strcpy(response, "Unknown command");
    }

    send_message(client_socket, response);
}

int run_chronotask(const char* config_file) {
    printf("Loading configuration...\n");
    if (!load_config(config_file)) {
        fprintf(stderr, "Failed to load configuration\n");
        return 1;
    }

    printf("Initializing tasks...\n");
    if (!initialize_tasks(config.default_task_list)) {
        fprintf(stderr, "Failed to initialize tasks\n");
        return 1;
    }

    printf("Initializing audio...\n");
    if (!initialize_audio()) {
        fprintf(stderr, "Failed to initialize audio\n");
        return 1;
    }

    printf("Opening display\n");
    if (!initialize_display()) {
        fprintf(stderr, "Failed to initialize display\n");
        cleanup_audio();
        return 1;
    }
    printf("Display opened\n");

    printf("Creating transparent window...\n");
    create_transparent_window();
    printf("Window created\n");

    printf("Creating command socket...\n");
    int command_socket = create_socket();
    if (command_socket == -1) {
        fprintf(stderr, "Failed to create command socket\n");
        cleanup_display();
        cleanup_audio();
        return 1;
    }
    printf("Command socket created\n");

    int flags = fcntl(command_socket, F_GETFL, 0);
    fcntl(command_socket, F_SETFL, flags | O_NONBLOCK);

    signal(SIGINT, handle_sigint);

    time_t task_start_time = time(NULL);
    printf("Entering main loop...\n");

    Routine *current_routine_ptr = &routine_list.routines[current_routine];

    while (keep_running) {
        time_t current_time = time(NULL);
        time_t elapsed_time;

        if (paused) {
            elapsed_time = difftime(pause_start_time, task_start_time) - total_pause_duration;
        } else {
            elapsed_time = difftime(current_time, task_start_time) - total_pause_duration;
        }

        draw_overlay(paused, elapsed_time);
        handle_x11_events();
    
        int client_socket = accept_connection(command_socket);
        if (client_socket != -1) {
            char buffer[BUFFER_SIZE];
            int bytes_read = receive_message(client_socket, buffer, BUFFER_SIZE);
            if (bytes_read > 0) {
                handle_command(client_socket, buffer);
            }
            close(client_socket);
        }
        
if (!paused && elapsed_time >= get_current_task_duration()) {
            printf("Task completed: %s\n", get_current_task_name());
            play_notification_sound();
            
            if (!move_to_next_task()) {
                if (current_routine_ptr->inf_loop || --current_routine_ptr->loop > 0) {
                    reset_routine();
                } else {
                    printf("Routine completed.\n");
                    break;
                }
            }
            
            task_start_time = time(NULL);
            total_pause_duration = 0;
            printf("Starting next task: %s\n", get_current_task_name());
        }

        usleep(10000);
    }

    printf("ChronoTask shutting down.\n");

    cleanup_display();
    cleanup_audio();
    close(command_socket);
    unlink(SOCKET_PATH);

    return 0;
}
