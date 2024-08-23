#include "window.h"
#include "error_report.h"
#include "config.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xinerama.h>
#include <stdio.h>
#include <stdlib.h>

Display *dpy = NULL;
Window win = 0;
GC gc = NULL;
int screen = 0;
Visual *visual = NULL;
int depth = 0;
XSetWindowAttributes attrs;

int initialize_display() {
    LOG_INFO("Initializing display...");
    
    dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
        LOG_ERROR("Cannot open display");
        return 0;
    }
    LOG_DEBUG("Display opened successfully");

    screen = DefaultScreen(dpy);
    LOG_DEBUG("Default screen: %d", screen);

    visual = DefaultVisual(dpy, screen);
    if (visual == NULL) {
        LOG_ERROR("Failed to get default Visual");
        XCloseDisplay(dpy);
        return 0;
    }
    LOG_DEBUG("Default Visual obtained");

    depth = DefaultDepth(dpy, screen);
    LOG_DEBUG("Default depth: %d", depth);

    gc = DefaultGC(dpy, screen);
    if (gc == NULL) {
        LOG_ERROR("Failed to get default Graphics Context");
        XCloseDisplay(dpy);
        return 0;
    }
    LOG_DEBUG("Default Graphics Context obtained");

    return 1;
}

void create_transparent_window() {
    XVisualInfo vinfo;
    XMatchVisualInfo(dpy, DefaultScreen(dpy), 32, TrueColor, &vinfo);
    visual = vinfo.visual;
    depth = vinfo.depth;

    screen = DefaultScreen(dpy);
    Window root = RootWindow(dpy, screen);

    attrs.colormap = XCreateColormap(dpy, root, visual, AllocNone);
    attrs.border_pixel = 0;
    attrs.background_pixel = 0;

    int num_screens;
    XineramaScreenInfo *screen_info = XineramaQueryScreens(dpy, &num_screens);
    if (screen_info == NULL || num_screens == 0) {
        LOG_ERROR("Xinerama is not active or no screens found");
        exit(1);
    }

    LOG_INFO("Detected %d screen(s)", num_screens);

    // Determine which screen to use based on config
    int screen_index = config.target_screen;
    if (screen_index < 0 || screen_index >= num_screens) {
        LOG_WARNING("Invalid target_screen in config. Using primary screen (0).");
        screen_index = 0;
    }

    int screen_width = screen_info[screen_index].width;
    int screen_height = screen_info[screen_index].height;
    int screen_x = screen_info[screen_index].x_org;
    int screen_y = screen_info[screen_index].y_org;

    XFree(screen_info);

    int width = config.window_width;
    int height = config.window_height;

    const int BOTTOM_OFFSET = 70;
    int x, y;

    switch (config.auto_x) {
        case H_LEFT:
            x = screen_x;
            break;
        case H_CENTER:
            x = screen_x + (screen_width - width) / 2;
            break;
        case H_RIGHT:
            x = screen_x + screen_width - width;
            break;
        case H_CUSTOM:
            x = (config.overlay_x < 0) ? screen_x + screen_width + config.overlay_x - width : screen_x + config.overlay_x;
            break;
    }

    LOG_INFO("Screen Width: %d, Screen Height: %d", screen_width, screen_height);
    LOG_INFO("Width: %d, Height: %d", width, height);
    LOG_INFO("screen x: %d, y: %d", screen_x, screen_y);
    switch (config.auto_y) {
        case V_TOP:
            y = screen_y;
            break;
        case V_MIDDLE:
            y = screen_y + (screen_height - height) / 2;
            break;
        case V_BOTTOM:
            y = screen_y + screen_height - height + BOTTOM_OFFSET;
            break;
        case V_CUSTOM:
            y = (config.overlay_y < 0) ? screen_y + screen_height + config.overlay_y - height : screen_y + config.overlay_y;
            break;
    }

    x = (x < screen_x) ? screen_x : x;
    x = (x + width > screen_x + screen_width) ? screen_x + screen_width - width : x;
    y = (y < screen_y) ? screen_y : y;
    y = (y + height > screen_y + screen_height) ? screen_y + screen_height - height : y;
    
    LOG_INFO("Creating window with dimensions: %dx%d at position (%d, %d)", width, height, x, y);

    win = XCreateWindow(dpy, root, x, y, width, height, 0, depth, InputOutput, visual,
                        CWColormap | CWBorderPixel | CWBackPixel, &attrs);

    if (win == 0) {
        LOG_ERROR("Failed to create window");
        exit(1);
    }

    Atom atom = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
    Atom value = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
    XChangeProperty(dpy, win, atom, XA_ATOM, 32, PropModeReplace, (unsigned char *)&value, 1);

    Atom wm_state = XInternAtom(dpy, "_NET_WM_STATE", False);
    Atom wm_top = XInternAtom(dpy, "_NET_WM_STATE_ABOVE", False);
    XChangeProperty(dpy, win, wm_state, XA_ATOM, 32, PropModeReplace, (unsigned char *)&wm_top, 1);

    XMapWindow(dpy, win);
    LOG_DEBUG("Window mapped");
}

void cleanup_display() {
    if (win) {
        XDestroyWindow(dpy, win);
    }
    if (dpy) {
        XCloseDisplay(dpy);
    }
    dpy = NULL;
    win = 0;
    gc = NULL;
    LOG_INFO("Display cleaned up");
}

void handle_x11_events() {
    XEvent ev;
    while (XPending(dpy)) {
        XNextEvent(dpy, &ev);
    }
}
