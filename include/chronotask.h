#ifndef CHRONOTASK_H
#define CHRONOTASK_H

#include <time.h>

int run_chronotask(const char* config_file);
void handle_command(int client_socket, const char* cmd);
int move_to_next_task(void);
void move_to_previous_task(void);
void extend_current_task(int seconds);
const char* get_current_task_name(void);
int get_current_task_duration(void);
time_t get_task_start_time(void);

#endif

