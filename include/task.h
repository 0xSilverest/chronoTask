#ifndef TASK_H
#define TASK_H

#include <time.h>

#define MAX_TASK_NAME 256
#define MAX_TASKS 100

typedef struct {
    char name[MAX_TASK_NAME];
    int duration;
} Task;

typedef struct {
    char name[MAX_TASK_NAME];
    Task tasks[MAX_TASKS];
    int task_count;
    int loop;
    int inf_loop;
} Routine;

int initialize_tasks(const char* task_list_name);
int move_to_next_task(void);
void move_to_previous_task(void);
void extend_current_task(int seconds);
const char* get_current_task_name(void);
int get_current_task_duration(void);
time_t get_task_start_time(void);

#endif
