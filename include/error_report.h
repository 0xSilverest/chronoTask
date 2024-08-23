#ifndef ERROR_REPORT_H
#define ERROR_REPORT_H

#include <stdio.h>
#include <stdlib.h>

typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_FATAL
} LogLevel;

extern LogLevel current_log_level;

void initialize_logging(const char* log_file);
void cleanup_logging(void);
void set_log_level(LogLevel level);
void log_message(LogLevel level, const char* file, int line, const char* format, ...);

#define LOG_DEBUG(...) if(current_log_level <= LOG_DEBUG) log_message(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...) if(current_log_level <= LOG_INFO) log_message(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARNING(...) if(current_log_level <= LOG_WARNING) log_message(LOG_WARNING, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) if(current_log_level <= LOG_ERROR) log_message(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_FATAL(...) do { log_message(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__); exit(EXIT_FAILURE); } while(0)

#endif
