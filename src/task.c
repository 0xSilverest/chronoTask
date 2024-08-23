#include "task.h"
#include "error_report.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>
#include <ctype.h>
#include <dirent.h>
#include <time.h>

RoutineList routine_list = {0};
int current_routine = -1;
static int current_task = 0;
static time_t task_start_time;


int parse_duration(const char* duration_str) {
    int total_seconds = 0;
    int value = 0;
    int len = strlen(duration_str);

    for (int i = 0; i < len; i++) {
        if (isdigit(duration_str[i])) {
            value = value * 10 + (duration_str[i] - '0');
        } else {
            switch (duration_str[i]) {
                case 'h':
                    total_seconds += value * 3600;
                    break;
                case 'm':
                    total_seconds += value * 60;
                    break;
                case 's':
                    total_seconds += value;
                    break;
            }
            value = 0;
        }
    }

    if (total_seconds == 0 && value != 0) {
        total_seconds = value;
    }

    return total_seconds;
}

int read_routines_from_file(const char* filename) {
    FILE *file = fopen(filename, "r");
    yaml_parser_t parser;
    yaml_event_t event;

    if (!file) {
        LOG_ERROR("Failed to open file %s", filename);
        return 0;
    }

    if (!yaml_parser_initialize(&parser)) {
        LOG_ERROR("Failed to initialize YAML parser");
        fclose(file);
        return 0;
    }

    yaml_parser_set_input_file(&parser, file);

    int in_routine = 0;
    int in_tasks = 0;
    int in_task = 0;
    char current_key[256] = "";
    Routine current_routine = {0};
    Task current_task = {0};

    do {
        if (!yaml_parser_parse(&parser, &event)) {
            fprintf(stderr, "Parser error %d\n", parser.error);
            yaml_parser_delete(&parser);
            fclose(file);
            return 0;
        }

        switch(event.type) {
            case YAML_SCALAR_EVENT:
                if (strcmp((char*)event.data.scalar.value, "routine-name") == 0) {
                    in_routine = 1;
                    strncpy(current_key, "routine-name", 255);
                } else if (strcmp((char*)event.data.scalar.value, "tasks") == 0) {
                    in_tasks = 1;
                } else if (in_routine && !in_tasks) {
                    if (strcmp((char*)event.data.scalar.value, "loop") == 0) {
                        strncpy(current_key, "loop", 255);
                    } else if (strcmp((char*)event.data.scalar.value, "inf-loop") == 0) {
                        strncpy(current_key, "inf-loop", 255);
                    } else if (current_key[0] != '\0') {
                        if (strcmp(current_key, "routine-name") == 0) {
                            strncpy(current_routine.name, (char*)event.data.scalar.value, MAX_TASK_NAME - 1);
                        } else if (strcmp(current_key, "loop") == 0) {
                            current_routine.loop = atoi((char*)event.data.scalar.value);
                        } else if (strcmp(current_key, "inf-loop") == 0) {
                            current_routine.inf_loop = (strcmp((char*)event.data.scalar.value, "true") == 0);
                        }
                        current_key[0] = '\0';
                    }
                } else if (in_tasks) {
                    if (strcmp((char*)event.data.scalar.value, "name") == 0 ||
                        strcmp((char*)event.data.scalar.value, "duration") == 0) {
                        strncpy(current_key, (char*)event.data.scalar.value, 255);
                    } else if (current_key[0] != '\0') {
                        if (strcmp(current_key, "name") == 0) {
                            strncpy(current_task.name, (char*)event.data.scalar.value, MAX_TASK_NAME - 1);
                        } else if (strcmp(current_key, "duration") == 0) {
                            current_task.duration = parse_duration((char*)event.data.scalar.value);
                        }
                        current_key[0] = '\0';
                    }
                }
                break;
            case YAML_MAPPING_START_EVENT:
                if (in_tasks) {
                    in_task = 1;
                    memset(&current_task, 0, sizeof(Task));
                }
                break;
            case YAML_MAPPING_END_EVENT:
                if (in_task) {
                    if (current_routine.task_count < MAX_TASKS) {
                        current_routine.tasks[current_routine.task_count++] = current_task;
                        printf("Added task: %s, duration: %d seconds\n", 
                               current_task.name, current_task.duration);
                    } else {
                        fprintf(stderr, "Maximum number of tasks reached for routine %s\n", current_routine.name);
                    }
                    in_task = 0;
                } else if (in_routine) {
                    if (routine_list.routine_count < MAX_ROUTINES) {
                        routine_list.routines[routine_list.routine_count++] = current_routine;
                        printf("Added routine: %s, tasks: %d, loop: %d, inf-loop: %s\n", 
                               current_routine.name, current_routine.task_count, 
                               current_routine.loop, current_routine.inf_loop ? "true" : "false");
                    } else {
                        fprintf(stderr, "Maximum number of routines reached\n");
                    }
                    in_routine = 0;
                    in_tasks = 0;
                    memset(&current_routine, 0, sizeof(Routine));
                }
                break;
            case YAML_SEQUENCE_END_EVENT:
                if (in_tasks) {
                    in_tasks = 0;
                }
                break;
            default:
                break;
        }

        if (event.type != YAML_STREAM_END_EVENT) {
            yaml_event_delete(&event);
        }
    } while (event.type != YAML_STREAM_END_EVENT);

    yaml_event_delete(&event);
    yaml_parser_delete(&parser);
    fclose(file);

    return routine_list.routine_count > 0;
}

int load_routines(const char* filename) {
    routine_list.routine_count = 0;
    return read_routines_from_file(filename);
}

int move_to_next_task(void) {
    Routine* current_routine_ptr = &routine_list.routines[current_routine];
    current_task++;
    if (current_task >= current_routine_ptr->task_count) {
        if (current_routine_ptr->inf_loop || current_routine_ptr->loop > 1) {
            current_task = 0;
            if (!current_routine_ptr->inf_loop) {
                current_routine_ptr->loop--;
            }
        } else {
            return 0;
        }
    }
    task_start_time = time(NULL);
    return 1;
}

void move_to_previous_task(void) {
    Routine* current_routine_ptr = &routine_list.routines[current_routine];
    if (current_task > 0) {
        current_task--;
    } else {
        current_task = current_routine_ptr->task_count - 1;
    }
    task_start_time = time(NULL);
}

void extend_current_task(int seconds) {
    Routine* current_routine_ptr = &routine_list.routines[current_routine];
    current_routine_ptr->tasks[current_task].duration += seconds;
}

const char* get_current_task_name(void) {
    Routine* current_routine_ptr = &routine_list.routines[current_routine];
    return current_routine_ptr->tasks[current_task].name;
}

int get_current_task_duration(void) {
    Routine* current_routine_ptr = &routine_list.routines[current_routine];
    return current_routine_ptr->tasks[current_task].duration;
}

time_t get_task_start_time(void) {
    return task_start_time;
}

int initialize_tasks() {
    if (current_routine < 0 || current_routine >= routine_list.routine_count) {
        LOG_ERROR("Invalid routine selected");
        return 0;
    }
    
    current_task = 0;
    task_start_time = time(NULL);
    LOG_INFO("Tasks initialized for routine: %s", routine_list.routines[current_routine].name);
    return 1;
}

int select_routine(const char* routine_name) {
    for (int i = 0; i < routine_list.routine_count; i++) {
        if (strcmp(routine_list.routines[i].name, routine_name) == 0) {
            current_routine = i;
            LOG_INFO("Routine selected: %s", routine_name);
            return 1;
        }
    }
    LOG_WARNING("Routine not found: %s", routine_name);
    return 0;
}

void list_routines() {
    printf("Available routines:\n");
    for (int i = 0; i < routine_list.routine_count; i++) {
        printf("%d. %s\n", i + 1, routine_list.routines[i].name);
    }
}

void reset_routine() {
    current_task = 0;
    task_start_time = time(NULL);
    LOG_INFO("Routine reset");
}
