#include "window.h"
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
    printf("Initializing display...\n");
    
    dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
        fprintf(stderr, "Cannot open display\n");
        return 0;
    }
    printf("Display opened successfully\n");

    screen = DefaultScreen(dpy);
    printf("Default screen: %d\n", screen);

    visual = DefaultVisual(dpy, screen);
    if (visual == NULL) {
        fprintf(stderr, "Failed to get default Visual\n");
        XCloseDisplay(dpy);
        return 0;
    }
    printf("Default Visual obtained\n");

    depth = DefaultDepth(dpy, screen);
    printf("Default depth: %d\n", depth);

    gc = DefaultGC(dpy, screen);
    if (gc == NULL) {
        fprintf(stderr, "Failed to get default Graphics Context\n");
        XCloseDisplay(dpy);
        return 0;
    }
    printf("Default Graphics Context obtained\n");

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
        fprintf(stderr, "Xinerama is not active\n");
        exit(1);
    }

    int screen_width = screen_info[0].width;
    int screen_height = screen_info[0].height;
    int screen_x = screen_info[0].x_org;
    int screen_y = screen_info[0].y_org;

    XFree(screen_info);

    int width = 500;
    int height = 100;

    int x = screen_x + (screen_width - width) / 2;
    int y = screen_y + screen_height - height - 20;
    
    printf("Creating window with dimensions: %dx%d at position (%d, %d)\n", width, height, x, y);

    win = XCreateWindow(dpy, root, x, y, width, height, 0, depth, InputOutput, visual,
                        CWColormap | CWBorderPixel | CWBackPixel, &attrs);

    if (win == 0) {
        fprintf(stderr, "Failed to create window\n");
        exit(1);
    }

    Atom atom = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
    Atom value = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
    XChangeProperty(dpy, win, atom, XA_ATOM, 32, PropModeReplace, (unsigned char *)&value, 1);

    Atom wm_state = XInternAtom(dpy, "_NET_WM_STATE", False);
    Atom wm_top = XInternAtom(dpy, "_NET_WM_STATE_ABOVE", False);
    XChangeProperty(dpy, win, wm_state, XA_ATOM, 32, PropModeReplace, (unsigned char *)&wm_top, 1);

    XMapWindow(dpy, win);
    printf("Window mapped\n");
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
}

void handle_x11_events() {
    XEvent ev;
    while (XPending(dpy)) {
        XNextEvent(dpy, &ev);
    }
}
