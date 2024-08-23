#include "config.h"
#include "error_report.h"
#include <yaml.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xft/Xft.h>
#include <unistd.h>
#include <pwd.h>

ChronoTaskConfig config;

char* get_config_path(const char* filename) {
    static char path[1024];
    char* home = getenv("HOME");
    
    if (!home) {
        struct passwd *pw = getpwuid(getuid());
        if (pw)
            home = pw->pw_dir;
    }

    if (home) {
        snprintf(path, sizeof(path), "%s/.config/chronotask/%s", home, filename);
        if (access(path, F_OK) != -1) return path;
    }

    snprintf(path, sizeof(path), "/usr/local/etc/chronotask/%s", filename);
    if (access(path, F_OK) != -1) return path;

    snprintf(path, sizeof(path), "./%s", filename);
    if (access(path, F_OK) != -1) return path;

    return NULL;
}

Color parse_color(const char* color_str) {
    Color color = {0, 0, 0};
    unsigned int r, g, b;
    if (sscanf(color_str, "#%02x%02x%02x", &r, &g, &b) == 3) {
        color.r = r;
        color.g = g;
        color.b = b;
    } else {
        LOG_WARNING("Invalid color format: %s. Using default (black).", color_str);
    }
    return color;
}

HorizontalPosition parse_horizontal_position(const char* pos_str) {
    if (strcasecmp(pos_str, "left") == 0) return H_LEFT;
    if (strcasecmp(pos_str, "center") == 0) return H_CENTER;
    if (strcasecmp(pos_str, "right") == 0) return H_RIGHT;
    return H_CUSTOM;
}

VerticalPosition parse_vertical_position(const char* pos_str) {
    if (strcasecmp(pos_str, "top") == 0) return V_TOP;
    if (strcasecmp(pos_str, "middle") == 0) return V_MIDDLE;
    if (strcasecmp(pos_str, "bottom") == 0) return V_BOTTOM;
    return V_CUSTOM;
}

int load_config(const char* filename) {
    char* config_path = get_config_path(filename);
    LOG_INFO("Holy path: %s", config_path);
    if (!config_path) {
        LOG_ERROR("Could not find configuration file: %s", filename);
        return 0;
    }

    FILE *file = fopen(config_path, "r");
    if (!file) {
        LOG_ERROR("Failed to open config file: %s", config_path);
        return 0;
    }

    yaml_parser_t parser;
    yaml_event_t event;

    if (!file) {
        LOG_ERROR("Failed to open config file: %s", filename);
        return 0;
    }

    if (!yaml_parser_initialize(&parser)) {
        LOG_ERROR("Failed to initialize YAML parser");
        fclose(file);
        return 0;
    }

    yaml_parser_set_input_file(&parser, file);

    char current_key[64] = "";

    do {
        if (!yaml_parser_parse(&parser, &event)) {
            LOG_ERROR("Parser error %d", parser.error);
            yaml_parser_delete(&parser);
            fclose(file);
            return 0;
        }

        switch(event.type) {
            case YAML_SCALAR_EVENT:
                if (current_key[0] == '\0') {
                    strncpy(current_key, (char*)event.data.scalar.value, sizeof(current_key) - 1);
                    current_key[sizeof(current_key) - 1] = '\0';
                } else {
                    if (strcmp(current_key, "notification_sound") == 0) {
                        strncpy(config.notification_sound, (char*)event.data.scalar.value, sizeof(config.notification_sound) - 1);
                        config.notification_sound[sizeof(config.notification_sound) - 1] = '\0';
                        LOG_DEBUG("Loaded notification_sound: %s", config.notification_sound);
                    } else if (strcmp(current_key, "overlay_x") == 0) {
                        config.overlay_x = atoi((char*)event.data.scalar.value);
                        LOG_DEBUG("Loaded overlay_x: %d", config.overlay_x);
                    } else if (strcmp(current_key, "overlay_y") == 0) {
                        config.overlay_y = atoi((char*)event.data.scalar.value);
                        LOG_DEBUG("Loaded overlay_y: %d", config.overlay_y);
                    } else if (strcmp(current_key, "font_size") == 0) {
                        config.font_size = atof((char*)event.data.scalar.value);
                        LOG_DEBUG("Loaded font_size: %.2f", config.font_size);
                    } else if (strcmp(current_key, "font_name") == 0) {
                        strncpy(config.font_name, (char*)event.data.scalar.value, sizeof(config.font_name) - 1);
                        config.font_name[sizeof(config.font_name) - 1] = '\0';
                        LOG_DEBUG("Loaded font_name: %s", config.font_name);
                    } else if (strcmp(current_key, "font_weight") == 0) {
                        config.font_weight = string_to_font_weight((char*)event.data.scalar.value);
                        LOG_DEBUG("Loaded font_weight: %d", config.font_weight);
                    } else if (strcmp(current_key, "routines") == 0) {
                        strncpy(config.routines, (char*)event.data.scalar.value, sizeof(config.routines) - 1);
                        config.routines[sizeof(config.routines) - 1] = '\0';
                        LOG_DEBUG("Loaded routines: %s", config.routines);
                    } else if (strcmp(current_key, "text_color") == 0) {
                        config.text_color = parse_color((char*)event.data.scalar.value);
                        LOG_DEBUG("Loaded text_color: #%02x%02x%02x", config.text_color.r, config.text_color.g, config.text_color.b);
                    } else if (strcmp(current_key, "stroke_color") == 0) {
                        config.stroke_color = parse_color((char*)event.data.scalar.value);
                        LOG_DEBUG("Loaded stroke_color: #%02x%02x%02x", config.stroke_color.r, config.stroke_color.g, config.stroke_color.b);
                    } else if (strcmp(current_key, "target_screen") == 0) {
                        config.target_screen = atoi((char*)event.data.scalar.value);
                        LOG_DEBUG("Loaded target_screen: %d", config.target_screen);
                    } else if (strcmp(current_key, "window_width") == 0) {
                        config.window_width = atoi((char*)event.data.scalar.value);
                        LOG_DEBUG("Loaded window_width: %d", config.window_width);
                    } else if (strcmp(current_key, "window_height") == 0) {
                        config.window_height = atoi((char*)event.data.scalar.value);
                        LOG_DEBUG("Loaded target_screen: %d", config.window_height);
                    }  else if (strcmp(current_key, "auto_x") == 0) {
                        config.auto_x = parse_horizontal_position((char*)event.data.scalar.value);
                        LOG_DEBUG("Loaded auto_x: %s", (char*)event.data.scalar.value);
                    } else if (strcmp(current_key, "auto_y") == 0) {
                        config.auto_y = parse_vertical_position((char*)event.data.scalar.value);
                        LOG_DEBUG("Loaded auto_y: %s", (char*)event.data.scalar.value);
                    } else {
                        LOG_WARNING("Unknown configuration key: %s", current_key);
                    }
                    current_key[0] = '\0';
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

    LOG_INFO("Configuration loaded successfully from %s", config_path);
    return 1;
}

int string_to_font_weight(const char *weight_str) {
    if (strcasecmp(weight_str, "Bold") == 0) {
        return XFT_WEIGHT_BOLD;
    } else if (strcasecmp(weight_str, "Black") == 0) {
        return XFT_WEIGHT_BLACK;
    } else if (strcasecmp(weight_str, "Heavy") == 0) {
        return FC_WEIGHT_HEAVY;
    } else if (strcasecmp(weight_str, "Medium") == 0) {
        return XFT_WEIGHT_MEDIUM;
    } else if (strcasecmp(weight_str, "Light") == 0) {
        return XFT_WEIGHT_LIGHT;
    } else if (strcasecmp(weight_str, "Thin") == 0) {
        return FC_WEIGHT_THIN;
    } else {
        LOG_WARNING("Unknown font weight '%s', using Normal", weight_str);
        return FC_WEIGHT_NORMAL;
    }
}
