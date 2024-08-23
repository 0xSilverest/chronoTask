#include "overlay.h"
#include "config.h"
#include "task.h"
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <string.h>
#include <stdio.h>

extern Display *dpy;
extern Window win;
extern GC gc;
extern int screen;
extern Visual *visual;
extern XSetWindowAttributes attrs;

void draw_stroke(char* display_text, XftColor xftcolor, XftFont *font, int text_x, int text_y, XftDraw *xftdraw) {
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (dx != 0 || dy != 0) {
                XftDrawStringUtf8(xftdraw, &xftcolor, font, text_x + dx, text_y + dy, (XftChar8 *)display_text, strlen(display_text));
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
        fprintf(stderr, "Failed to get window attributes\n");
        return;
    }
    int width = wa.width;
    int height = wa.height;

    XSetForeground(dpy, gc, 0x00000000);
    XClearWindow(dpy, win);

    int remaining = get_current_task_duration() - elapsed_time;
    
    char time_str[20];
    format_time(remaining, time_str, sizeof(time_str));
    
    const char* task_name = get_current_task_name();
    const char* separator = " - ";
    const char* paused_text = is_paused ? " (PAUSED)" : "";

    // Create Xft draw context
    XftDraw *xftdraw = XftDrawCreate(dpy, win, visual, attrs.colormap);
    if (!xftdraw) {
        fprintf(stderr, "Failed to create XftDraw\n");
        return;
    }
    
    XftFont *font = XftFontOpen(dpy, screen,
                                XFT_FAMILY, XftTypeString, config.font_name,
                                XFT_SIZE, XftTypeDouble, config.font_size,
                                XFT_WEIGHT, XftTypeInteger, config.font_weight,
                                XFT_ANTIALIAS, XftTypeBool, True,
                                FC_HINTING, XftTypeBool, True,
                                NULL);
    if (!font) {
        fprintf(stderr, "Failed to load font: %s, size %f\n", config.font_name, config.font_size);
        XftDrawDestroy(xftdraw);
        return;
    }

    XftColor xftcolor_white, xftcolor_black;
    XRenderColor xrcolor_white = {65535, 65535, 65535, 65535};
    XRenderColor xrcolor_black = {0, 0, 0, 65535};
    XftColorAllocValue(dpy, visual, attrs.colormap, &xrcolor_white, &xftcolor_white);
    XftColorAllocValue(dpy, visual, attrs.colormap, &xrcolor_black, &xftcolor_black);

    XGlyphInfo extents_task, extents_separator, extents_time, extents_paused;
    XftTextExtentsUtf8(dpy, font, (XftChar8 *)task_name, strlen(task_name), &extents_task);
    XftTextExtentsUtf8(dpy, font, (XftChar8 *)separator, strlen(separator), &extents_separator);
    XftTextExtentsUtf8(dpy, font, (XftChar8 *)time_str, strlen(time_str), &extents_time);
    XftTextExtentsUtf8(dpy, font, (XftChar8 *)paused_text, strlen(paused_text), &extents_paused);

    int total_width = extents_task.xOff + extents_separator.xOff + extents_time.xOff + extents_paused.xOff;

    int text_x = (width - total_width) / 2;
    int text_y = (height + extents_task.height) / 2 - extents_task.y;

    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (dx != 0 || dy != 0) {
                XftDrawStringUtf8(xftdraw, &xftcolor_black, font, text_x + dx, text_y + dy, (XftChar8 *)task_name, strlen(task_name));
            }
        }
    }

    XftDrawStringUtf8(xftdraw, &xftcolor_white, font, text_x, text_y, (XftChar8 *)task_name, strlen(task_name));
    text_x += extents_task.xOff;

    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (dx != 0 || dy != 0) {
                XftDrawStringUtf8(xftdraw, &xftcolor_black, font, text_x + dx, text_y + dy, (XftChar8 *)separator, strlen(separator));
            }
        }
    }
    XftDrawStringUtf8(xftdraw, &xftcolor_white, font, text_x, text_y, (XftChar8 *)separator, strlen(separator));
    text_x += extents_separator.xOff;

    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (dx != 0 || dy != 0) {
                XftDrawStringUtf8(xftdraw, &xftcolor_black, font, text_x + dx, text_y + dy, (XftChar8 *)time_str, strlen(time_str));
            }
        }
    }
    XftDrawStringUtf8(xftdraw, &xftcolor_white, font, text_x, text_y, (XftChar8 *)time_str, strlen(time_str));
    text_x += extents_time.xOff;

    if (is_paused) {
        draw_stroke(strdup(paused_text), xftcolor_black, font, text_x, text_y, xftdraw);
        XftDrawStringUtf8(xftdraw, &xftcolor_white, font, text_x, text_y, (XftChar8 *)paused_text, strlen(paused_text));
    }

    XftColorFree(dpy, visual, attrs.colormap, &xftcolor_white);
    XftFontClose(dpy, font);
    XftDrawDestroy(xftdraw);

    XFlush(dpy);
}
