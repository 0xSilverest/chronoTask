#ifndef OVERLAY_H
#define OVERLAY_H

#include <time.h>

void draw_overlay(int is_paused, time_t elapsed_time);
void cleanup_overlay_resources(void);

#endif

