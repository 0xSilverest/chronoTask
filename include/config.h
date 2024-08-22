#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    char notification_sound[256];
    int overlay_x;
    int overlay_y;
    double font_size;
    char font_name[64];
    char default_task_list[64];
    int font_weight;
} ChronoTaskConfig;

extern ChronoTaskConfig config;

int load_config(const char* filename);

#endif
