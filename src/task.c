#include "task.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>
#include <ctype.h>

static Routine routine = {0};
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

int read_tasks_from_file(const char* filename) {
    FILE *file = fopen(filename, "r");
    yaml_parser_t parser;
    yaml_event_t event;

    if (!file) {
        fprintf(stderr, "Failed to open file %s\n", filename);
        return 0;
    }

    if (!yaml_parser_initialize(&parser)) {
        fprintf(stderr, "Failed to initialize parser\n");
        fclose(file);
        return 0;
    }

    yaml_parser_set_input_file(&parser, file);

    int in_routine = 0;
    int in_tasks = 0;
    int in_task = 0;
    char current_key[256] = "";
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
                if (!in_routine) {
                    strncpy(routine.name, (char*)event.data.scalar.value, MAX_TASK_NAME - 1);
                    in_routine = 1;
                } else if (strcmp((char*)event.data.scalar.value, "tasks") == 0) {
                    in_tasks = 1;
                } else if (!in_tasks) {
                    if (strcmp((char*)event.data.scalar.value, "loop") == 0) {
                        strncpy(current_key, "routine_loop", 255);
                    } else if (strcmp((char*)event.data.scalar.value, "inf-loop") == 0) {
                        strncpy(current_key, "routine_inf_loop", 255);
                    } else if (current_key[0] != '\0') {
                        if (strcmp(current_key, "routine_loop") == 0) {
                            routine.loop = atoi((char*)event.data.scalar.value);
                        } else if (strcmp(current_key, "routine_inf_loop") == 0) {
                            routine.inf_loop = (strcmp((char*)event.data.scalar.value, "true") == 0);
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
                    if (routine.task_count < MAX_TASKS) {
                        routine.tasks[routine.task_count++] = current_task;
                        printf("Added task: %s, duration: %d seconds\n", 
                               current_task.name, current_task.duration);
                    } else {
                        fprintf(stderr, "Maximum number of tasks reached\n");
                    }
                    in_task = 0;
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

    printf("Read routine: %s, tasks: %d, loop: %d, inf-loop: %s\n", 
           routine.name, routine.task_count, routine.loop, routine.inf_loop ? "true" : "false");
    return 1;
}

int move_to_next_task(void) {
    current_task++;
    if (current_task >= routine.task_count) {
        if (routine.inf_loop || routine.loop > 1) {
            current_task = 0;
            if (!routine.inf_loop) {
                routine.loop--;
            }
        } else {
            return 0;
        }
    }
    task_start_time = time(NULL);
    return 1;
}

void move_to_previous_task(void) {
    if (current_task > 0) {
        current_task--;
    } else {
        current_task = routine.task_count - 1;
    }
    task_start_time = time(NULL);
}

void extend_current_task(int seconds) {
    routine.tasks[current_task].duration += seconds;
}

const char* get_current_task_name(void) {
    return routine.tasks[current_task].name;
}

int get_current_task_duration(void) {
    return routine.tasks[current_task].duration;
}

time_t get_task_start_time(void) {
    return task_start_time;
}

int initialize_tasks(const char* task_list_name) {
    char filename[256];
    snprintf(filename, sizeof(filename), "%s.yaml", task_list_name);
    
    if (!read_tasks_from_file(filename)) {
        fprintf(stderr, "Failed to read tasks from file: %s\n", filename);
        return 0;
    }
    
    current_task = 0;
    task_start_time = time(NULL);
    return 1;
}
