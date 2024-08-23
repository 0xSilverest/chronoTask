#include "config.h"
#include <yaml.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xft/Xft.h>

ChronoTaskConfig config;

int load_config(const char* filename) {
    FILE *file = fopen(filename, "r");
    yaml_parser_t parser;
    yaml_event_t event;

    if (!file) {
        fprintf(stderr, "Failed to open config file: %s\n", filename);
        return 0;
    }

    if (!yaml_parser_initialize(&parser)) {
        fprintf(stderr, "Failed to initialize YAML parser\n");
        fclose(file);
        return 0;
    }

    yaml_parser_set_input_file(&parser, file);

    char current_key[64] = "";

    do {
        if (!yaml_parser_parse(&parser, &event)) {
            fprintf(stderr, "Parser error %d\n", parser.error);
            yaml_parser_delete(&parser);
            fclose(file);
            return 0;
        }

        switch(event.type) {
            case YAML_SCALAR_EVENT:
                if (current_key[0] == '\0') {
                    strncpy(current_key, (char*)event.data.scalar.value, sizeof(current_key) - 1);
                } else {
                    if (strcmp(current_key, "notification_sound") == 0) {
                        strncpy(config.notification_sound, (char*)event.data.scalar.value, sizeof(config.notification_sound) - 1);
                    } else if (strcmp(current_key, "overlay_x") == 0) {
                        config.overlay_x = atoi((char*)event.data.scalar.value);
                    } else if (strcmp(current_key, "overlay_y") == 0) {
                        config.overlay_y = atoi((char*)event.data.scalar.value);
                    } else if (strcmp(current_key, "font_size") == 0) {
                        config.font_size = atof((char*)event.data.scalar.value);
                    } else if (strcmp(current_key, "font_name") == 0) {
                        strncpy(config.font_name, (char*)event.data.scalar.value, sizeof(config.font_name) - 1);
                    } else if (strcmp(current_key, "font_weight") == 0) {
                        config.font_weight = string_to_font_weight((char*)event.data.scalar.value);
                    } else if (strcmp(current_key, "routines") == 0) {
                        strncpy(config.routines, (char*)event.data.scalar.value, sizeof(config.routines) - 1);
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

    printf("Configuration loaded successfully\n");
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
        return FC_WEIGHT_NORMAL;
    }
}
