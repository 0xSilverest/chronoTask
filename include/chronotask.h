#ifndef CHRONOTASK_H
#define CHRONOTASK_H

#define _POSIX_C_SOURCE 199309L
#include <time.h>

int run_chronotask(const char* config_file);
void handle_command(int client_socket, const char* cmd);
const char* get_current_task_name(void);
int get_current_task_duration(void);
time_t get_task_start_time(void);
int move_to_next_task(void);
void move_to_previous_task(void);
void extend_current_task(int seconds);

#endif

