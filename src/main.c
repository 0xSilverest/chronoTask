#include "task.h"
#include "config.h"
#include "chronotask.h"
#include <stdio.h>

   int main(int argc, char *argv[]) {
    const char* config_file = "config.yaml";
    
    if (!load_config(config_file)) {
        fprintf(stderr, "Failed to load configuration\n");
        return 1;
    }
    
    char routines_file[512];
    snprintf(routines_file, sizeof(routines_file), "%s.yaml", config.routines);
    
    if (!load_routines(routines_file)) {
        fprintf(stderr, "Failed to load routines from file: %s\n", routines_file);
        return 1;
    }
    
    if (argc > 1) {
        if (!select_routine(argv[1])) {
            fprintf(stderr, "Routine '%s' not found\n", argv[1]);
            list_routines();
            return 1;
        }
    } else if (routine_list.routine_count == 1) {
        current_routine = 0;
    } else {
        list_routines();
        int choice;
        printf("Enter the number of the routine to run: ");
        if (scanf("%d", &choice) != 1 || choice < 1 || choice > routine_list.routine_count) {
            fprintf(stderr, "Invalid choice\n");
            return 1;
        }
        current_routine = choice - 1;
    }
    
    if (!initialize_tasks()) {
        fprintf(stderr, "Failed to initialize tasks\n");
        return 1;
    }
     
    printf("Starting ChronoTask with routine: %s\n", routine_list.routines[current_routine].name);
    int result = run_chronotask(config_file);
    printf("ChronoTask exited with result: %d\n", result);
    return result;
}

