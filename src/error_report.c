#include "error_report.h"
#include <stdarg.h>
#include <time.h>
#include <string.h>

static FILE* log_file = NULL;
static const char* level_strings[] = {
    "DEBUG", "INFO", "WARNING", "ERROR", "FATAL"
};
LogLevel current_log_level = LOG_ERROR;

void initialize_logging(const char* log_filename) {
    log_file = fopen(log_filename, "a");
    if (log_file == NULL) {
        fprintf(stderr, "Failed to open log file: %s\n", log_filename);
        exit(EXIT_FAILURE);
    }
}

void cleanup_logging(void) {
    if (log_file != NULL) {
        fclose(log_file);
        log_file = NULL;
    }
}

void set_log_level(LogLevel level) {
    current_log_level = level;
}

void log_message(LogLevel level, const char* file, int line, const char* format, ...) {
    if (level < current_log_level) {
        return;
    }

    va_list args;
    char buffer[1024];
    time_t now;
    char time_buffer[26];
    struct tm* tm_info;

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    now = time(NULL);
    tm_info = localtime(&now);
    strftime(time_buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);

    if (log_file != NULL) {
        fprintf(log_file, "[%s] %s %s:%d: %s\n", time_buffer, level_strings[level], file, line, buffer);
        fflush(log_file);
    }

    if (level >= current_log_level) {
        fprintf(stderr, "[%s] %s %s:%d: %s\n", time_buffer, level_strings[level], file, line, buffer);
    }

    if (level == LOG_FATAL) {
        exit(EXIT_FAILURE);
    }
}
