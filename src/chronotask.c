#include "chronotask.h"
#include "error_report.h"
#include "config.h"
#include "task.h"
#include "window.h"
#include "overlay.h"
#include "audio.h"
#include "socket.h"
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

volatile sig_atomic_t keep_running = 1;
int paused = 0;
time_t pause_start_time = 0;
time_t total_pause_duration = 0;
time_t task_start_time = 0;

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
        int moved = move_to_next_task();
        if (moved) {
            task_start_time = time(NULL);
            total_pause_duration = 0;
        }
        snprintf(response, BUFFER_SIZE, "Moved to next task: %s", get_current_task_name());
    } else if (strcmp(cmd, "previous") == 0) {
        move_to_previous_task();
        task_start_time = time(NULL);
        total_pause_duration = 0;
        snprintf(response, BUFFER_SIZE, "Moved to previous task: %s", get_current_task_name());
    } else if (strncmp(cmd, "extend ", 7) == 0) {
        int minutes = atoi(cmd + 7);
        extend_current_task(minutes * 60);
        snprintf(response, BUFFER_SIZE, "Extended task by %d minutes", minutes);
    } else if (strcmp(cmd, "status") == 0) {
        time_t now = time(NULL);
        int elapsed = difftime(now, get_task_start_time()) - total_pause_duration;
        int remaining = get_current_task_duration() - elapsed;
        snprintf(response, BUFFER_SIZE, "Current task: %s, Time remaining: %d seconds, Status: %s",
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
    LOG_INFO("Loading configuration...");
    if (!load_config(config_file)) {
        LOG_FATAL("Failed to load configuration from %s", config_file);
    }

    LOG_INFO("Initializing routines...");
    if (!initialize_tasks()) {
        LOG_FATAL("Failed to initialize routines");
    }

    LOG_INFO("Initializing audio...");
    if (!initialize_audio()) {
        LOG_WARNING("Failed to initialize audio. ChronoTask will continue without sound.");
    }

    LOG_INFO("Opening display");
    if (!initialize_display()) {
        LOG_FATAL("Failed to initialize display");
    }

    LOG_INFO("Creating transparent window...");
    create_transparent_window();

    LOG_INFO("Creating command socket...");
    int command_socket = create_socket();
    if (command_socket == -1) {
        LOG_FATAL("Failed to create command socket");
    }

    int flags = fcntl(command_socket, F_GETFL, 0);
    fcntl(command_socket, F_SETFL, flags | O_NONBLOCK);

    signal(SIGINT, handle_sigint);

    task_start_time = time(NULL);
    LOG_INFO("Entering main loop...");

    Routine *current_routine_ptr = &routine_list.routines[current_routine];

    while (keep_running) {
        time_t current_time = time(NULL);
        time_t elapsed_time;

        elapsed_time = paused ?
            difftime(pause_start_time, task_start_time) - total_pause_duration :
            difftime(current_time, task_start_time) - total_pause_duration;

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
            LOG_INFO("Task completed: %s", get_current_task_name());
            play_notification_sound();

            if (!move_to_next_task()) {
                if (current_routine_ptr->inf_loop || --current_routine_ptr->loop > 0) {
                    reset_routine();
                } else {
                    LOG_INFO("Routine completed.");
                    break;
                }
            }

            task_start_time = time(NULL);
            total_pause_duration = 0;
            LOG_INFO("Starting next task: %s", get_current_task_name());
        }

        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 10000000L;
        nanosleep(&ts, NULL);
    }

    LOG_INFO("ChronoTask shutting down.");

    cleanup_display();
    cleanup_audio();
    close(command_socket);
    unlink(SOCKET_PATH);

    return 0;
}
