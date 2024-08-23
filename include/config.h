#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h> 

typedef enum {
    H_LEFT,
    H_CENTER,
    H_RIGHT,
    H_CUSTOM
} HorizontalPosition;

typedef enum {
    V_TOP,
    V_MIDDLE,
    V_BOTTOM,
    V_CUSTOM
} VerticalPosition;

typedef struct {
    unsigned char r, g, b;
} Color;

typedef struct {
    char notification_sound[256];
    int overlay_x;
    int overlay_y;
    double font_size;
    char font_name[64];
    int font_weight;
    char routines[256];
    Color text_color;
    Color stroke_color;
    int target_screen;
    int window_width;
    int window_height;
    HorizontalPosition auto_x;
    VerticalPosition auto_y;
} ChronoTaskConfig;

extern ChronoTaskConfig config;

int load_config(const char* filename);
int string_to_font_weight(const char *weight_str);
Color parse_color(const char* color_str);
char* get_config_path(const char* filename);

#endif
