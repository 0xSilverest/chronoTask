#include "overlay.h"
#include "config.h"
#include "task.h"
#include "error_report.h"
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

extern Display *dpy;
extern Window win;
extern GC gc;
extern int screen;
extern Visual *visual;
extern XSetWindowAttributes attrs;

static XftFont *cached_font = NULL;
static XftColor cached_color_white, cached_color_black;
static int colors_allocated = 0;

static int warning_logged = 0;
static char last_task_name[256] = "";

static int last_paused_state = -1;
static int debug_logged = 0;

void cleanup_overlay_resources() {
    if (cached_font) {
        XftFontClose(dpy, cached_font);
        cached_font = NULL;
    }
    if (colors_allocated) {
        XftColorFree(dpy, visual, attrs.colormap, &cached_color_white);
        XftColorFree(dpy, visual, attrs.colormap, &cached_color_black);
        colors_allocated = 0;
    }
}

int initialize_overlay_resources() {
    if (!cached_font) {
        cached_font = XftFontOpen(dpy, screen,
                                  XFT_FAMILY, XftTypeString, config.font_name,
                                  XFT_SIZE, XftTypeDouble, config.font_size,
                                  XFT_WEIGHT, XftTypeInteger, config.font_weight,
                                  XFT_ANTIALIAS, XftTypeBool, True,
                                  FC_HINTING, XftTypeBool, True,
                                  NULL);
        if (!cached_font) {
            LOG_ERROR("Failed to load font: %s, size %f", config.font_name, config.font_size);
            return 0;
        }
    }

    if (!colors_allocated) {
        XRenderColor xrcolor_white = {65535, 65535, 65535, 65535};
        XRenderColor xrcolor_black = {0, 0, 0, 65535};
        if (!XftColorAllocValue(dpy, visual, attrs.colormap, &xrcolor_white, &cached_color_white) ||
            !XftColorAllocValue(dpy, visual, attrs.colormap, &xrcolor_black, &cached_color_black)) {
            LOG_ERROR("Failed to allocate colors");
            cleanup_overlay_resources();
            return 0;
        }
        colors_allocated = 1;
    }

    return 1;
}

void draw_stroke(const char* display_text, XftColor xftcolor, int text_x, int text_y, XftDraw *xftdraw) {
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (dx != 0 || dy != 0) {
                XftDrawStringUtf8(xftdraw, &xftcolor, cached_font, text_x + dx, text_y + dy, (XftChar8 *)display_text, strlen(display_text));
            }
        }
    }
}

void format_time(int seconds, char *buffer, size_t bufsize) {
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    int secs = seconds % 60;

    if (hours > 0) {
        snprintf(buffer, bufsize, "%02d:%02d:%02d", hours, minutes, secs);
    } else {
        snprintf(buffer, bufsize, "%02d:%02d", minutes, secs);
    }
}

void draw_overlay(int is_paused, time_t elapsed_time) {
    XWindowAttributes wa;
    if (XGetWindowAttributes(dpy, win, &wa) == 0) {
        LOG_ERROR("Failed to get window attributes");
        return;
    }
    int width = wa.width;
    int height = wa.height;

    if (!initialize_overlay_resources()) {
        LOG_ERROR("Failed to initialize overlay resources");
        return;
    }

    XSetForeground(dpy, gc, 0x00000000);
    XClearWindow(dpy, win);

    int remaining = get_current_task_duration() - elapsed_time;
    
    char time_str[20];
    format_time(remaining, time_str, sizeof(time_str));
    
    const char* task_name = get_current_task_name();
    const char* separator = " - ";
    const char* paused_text = is_paused ? " (PAUSED)" : "";

    XftDraw *xftdraw = XftDrawCreate(dpy, win, visual, attrs.colormap);
    if (!xftdraw) {
        LOG_ERROR("Failed to create XftDraw");
        return;
    }

    XGlyphInfo extents_task, extents_separator, extents_time, extents_paused;
    XftTextExtentsUtf8(dpy, cached_font, (XftChar8 *)task_name, strlen(task_name), &extents_task);
    XftTextExtentsUtf8(dpy, cached_font, (XftChar8 *)separator, strlen(separator), &extents_separator);
    XftTextExtentsUtf8(dpy, cached_font, (XftChar8 *)time_str, strlen(time_str), &extents_time);
    XftTextExtentsUtf8(dpy, cached_font, (XftChar8 *)paused_text, strlen(paused_text), &extents_paused); 

    if (strcmp(last_task_name, task_name) != 0) {
        strncpy(last_task_name, task_name, sizeof(last_task_name) - 1);
        last_task_name[sizeof(last_task_name) - 1] = '\0';
        warning_logged = 0;
    }

    if (!warning_logged && (extents_task.width == 0 || extents_separator.width == 0 || 
        extents_time.width == 0 || extents_paused.width == 0)) {
        LOG_WARNING("One or more text extents have zero width. This might indicate a problem with the font or text.");
        warning_logged = 1;
    }

    int total_width = extents_task.xOff + extents_separator.xOff + extents_time.xOff + extents_paused.xOff;

    int text_x = (width - total_width) / 2;
    int text_y = (height + extents_task.height) / 2 - extents_task.y;

    draw_stroke(task_name, cached_color_black, text_x, text_y, xftdraw);
    XftDrawStringUtf8(xftdraw, &cached_color_white, cached_font, text_x, text_y, (XftChar8 *)task_name, strlen(task_name));
    text_x += extents_task.xOff;

    draw_stroke(separator, cached_color_black, text_x, text_y, xftdraw);
    XftDrawStringUtf8(xftdraw, &cached_color_white, cached_font, text_x, text_y, (XftChar8 *)separator, strlen(separator));
    text_x += extents_separator.xOff;

    draw_stroke(time_str, cached_color_black, text_x, text_y, xftdraw);
    XftDrawStringUtf8(xftdraw, &cached_color_white, cached_font, text_x, text_y, (XftChar8 *)time_str, strlen(time_str));
    text_x += extents_time.xOff;

    if (is_paused) {
        draw_stroke(paused_text, cached_color_black, text_x, text_y, xftdraw);
        XftDrawStringUtf8(xftdraw, &cached_color_white, cached_font, text_x, text_y, (XftChar8 *)paused_text, strlen(paused_text));
    }
    XftDrawDestroy(xftdraw);


    if (strcmp(last_task_name, task_name) != 0 || last_paused_state != is_paused) {
        strncpy(last_task_name, task_name, sizeof(last_task_name) - 1);
        last_task_name[sizeof(last_task_name) - 1] = '\0';
        last_paused_state = is_paused;
        debug_logged = 0;
    }
    
    if (!debug_logged) {
        LOG_DEBUG("Overlay updated - Task: %s, State: %s", task_name, is_paused ? "Paused" : "Running");
        debug_logged = 1;
    }

    XFlush(dpy);
}
