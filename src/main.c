#include "task.h"
#include "config.h"
#include "chronotask.h"
#include "error_report.h"
#include "routine_selector.h"
#include <string.h>
#include <stdio.h>

LogLevel parse_log_level(const char* level_str) {
    if (level_str == NULL || strcmp(level_str, "info") == 0) {
        return LOG_INFO;
    } else if (strcmp(level_str, "debug") == 0) {
        return LOG_DEBUG;
    } else if (strcmp(level_str, "warning") == 0) {
        return LOG_WARNING;
    } else if (strcmp(level_str, "error") == 0) {
        return LOG_ERROR;
    } else {
        fprintf(stderr, "Unknown log level: %s. Using INFO.\n", level_str);
        return LOG_INFO;
    }
}

int main(int argc, char *argv[]) {
    const char* config_file = "config.yaml";
    const char* routine_name = NULL;
    LogLevel log_level = LOG_ERROR;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--verbose") == 0) {
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                log_level = parse_log_level(argv[++i]);
            } else {
                log_level = LOG_INFO;
            }
        } else if (strncmp(argv[i], "--verbose=", 10) == 0) {
            log_level = parse_log_level(argv[i] + 10);
        } else if (routine_name == NULL) {
            routine_name = argv[i];
        }
    }

    initialize_logging("chronoTask.log");
    set_log_level(log_level);

    LOG_INFO("ChronoTask starting up");

    if (!load_config(config_file)) {
        LOG_FATAL("Failed to load configuration from %s", config_file);
    }

    char routines_file[512];
    snprintf(routines_file, sizeof(routines_file), "%s.yaml", config.routines);

    if (!load_routines(routines_file)) {
        LOG_FATAL("Failed to load routines from file: %s", routines_file);
    }

    if (routine_name == NULL) {
        int selected = select_routine_gui(&routine_list);
        if (selected >= 0) {
            current_routine = selected;
        } else {
            LOG_INFO("User cancelled routine selection");
            cleanup_logging();
            return 1;
        }
    } else {
        if (!select_routine(routine_name)) {
            LOG_ERROR("Routine '%s' not found", routine_name);
            list_routines();
            cleanup_logging();
            return 1;
        }
    }

    if (!initialize_tasks()) {
        LOG_FATAL("Failed to initialize tasks");
    }

    LOG_INFO("Starting ChronoTask with routine: %s", routine_list.routines[current_routine].name);
    int result = run_chronotask(config_file);
    LOG_INFO("ChronoTask exited with result: %d", result);

    cleanup_logging();
    return result;
}
