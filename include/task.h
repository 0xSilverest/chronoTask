#ifndef TASK_H
#define TASK_H

#include <time.h>

#define MAX_TASK_NAME 256
#define MAX_TASKS 100
#define MAX_ROUTINES 10

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

typedef struct {
    Routine routines[MAX_ROUTINES];
    int routine_count;
} RoutineList;

extern RoutineList routine_list;
extern int current_routine;

int load_routines(const char* directory);
int select_routine(const char* routine_name);
void list_routines();
void reset_routine();
int initialize_tasks();
int move_to_next_task(void);
void move_to_previous_task(void);
void extend_current_task(int seconds);
const char* get_current_task_name(void);
int get_current_task_duration(void);
time_t get_task_start_time(void);

#endif
