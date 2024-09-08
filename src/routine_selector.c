#include "routine_selector.h"
#include "config.h"
#include "error_report.h"
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define WINDOW_WIDTH 500
#define ROUTINE_HEIGHT 50
#define PADDING 20

static Display *dpy;
static int screen;
static Window win;
static GC gc;
static XftFont *font;
static XftDraw *xft_draw;
static XftColor text_color, highlight_color, bg_color;

static int selected_index = 0;

static void create_window(int height) {
    dpy = XOpenDisplay(NULL);
    if (!dpy) {
        LOG_FATAL("Cannot open display");
    }

    screen = DefaultScreen(dpy);

    win = XCreateSimpleWindow(dpy, RootWindow(dpy, screen),
                              0, 0, WINDOW_WIDTH, height,
                              1, BlackPixel(dpy, screen), WhitePixel(dpy, screen));

    XStoreName(dpy, win, "ChronoTask");

    XSelectInput(dpy, win, ExposureMask | KeyPressMask | ButtonPressMask);
    XMapWindow(dpy, win);

    gc = XCreateGC(dpy, win, 0, NULL);

    XWindowAttributes wa;
    XGetWindowAttributes(dpy, RootWindow(dpy, screen), &wa);
    int x = (wa.width - WINDOW_WIDTH) / 2;
    int y = (wa.height - height) / 2;
    XMoveWindow(dpy, win, x, y);
}


static void init_font_and_colors() {
    Visual *visual = DefaultVisual(dpy, screen);
    Colormap cmap = DefaultColormap(dpy, screen);

    font = XftFontOpen(dpy, screen,
                       XFT_FAMILY, XftTypeString, config.menu_font_name,
                       XFT_SIZE, XftTypeDouble, config.menu_font_size,
                       NULL);

    xft_draw = XftDrawCreate(dpy, win, visual, cmap);

    XRenderColor xre_text = {config.menu_text_color.r << 8, config.menu_text_color.g << 8, config.menu_text_color.b << 8, 0xFFFF};
    XRenderColor xre_highlight = {config.menu_highlight_color.r << 8, config.menu_highlight_color.g << 8, config.menu_highlight_color.b << 8, 0xFFFF};
    XRenderColor xre_bg = {config.menu_bg_color.r << 8, config.menu_bg_color.g << 8, config.menu_bg_color.b << 8, 0xFFFF};

    XftColorAllocValue(dpy, visual, cmap, &xre_text, &text_color);
    XftColorAllocValue(dpy, visual, cmap, &xre_highlight, &highlight_color);
    XftColorAllocValue(dpy, visual, cmap, &xre_bg, &bg_color);
}

static void draw_routines(RoutineList *routines) {
    XClearWindow(dpy, win);

    XSetForeground(dpy, gc, bg_color.pixel);
    XFillRectangle(dpy, win, gc, 0, 0, WINDOW_WIDTH, routines->routine_count * ROUTINE_HEIGHT);

    for (int i = 0; i < routines->routine_count; i++) {
        int y = i * ROUTINE_HEIGHT;

        if (i == selected_index) {
            XSetForeground(dpy, gc, highlight_color.pixel);
            XFillRectangle(dpy, win, gc, 0, y, WINDOW_WIDTH, ROUTINE_HEIGHT);
        }
        XGlyphInfo extents;

        XftTextExtentsUtf8(dpy, font,
            (XftChar8 *)routines->routines[i].name,
            strlen(routines->routines[i].name), &extents);

        XftDrawStringUtf8(xft_draw, &text_color, font,
            (WINDOW_WIDTH - extents.xOff) / 2,
            y + (ROUTINE_HEIGHT + font->ascent - font->descent) / 2,
            (XftChar8 *)routines->routines[i].name,
            strlen(routines->routines[i].name));
    }
}

static int handle_events(RoutineList *routines) {
    XEvent ev;
    KeySym key;

    while (1) {
        XNextEvent(dpy, &ev);

        switch (ev.type) {
            case Expose:
                draw_routines(routines);
                break;

            case KeyPress:
                key = XLookupKeysym(&ev.xkey, 0);

                if (key == XK_Q || key == XK_Escape) return -1;
                else if (key == XK_Return) return selected_index;
                else if ((key == XK_Up || key==XK_k) && selected_index > 0) {
                    selected_index--;
                    draw_routines(routines);
                } else if ((key == XK_Down || key==XK_j) && selected_index < routines->routine_count - 1) {
                    selected_index++;
                    draw_routines(routines);
                }
                break;

            case ButtonPress:
                if (ev.xbutton.button == Button1) {
                    int new_index = ev.xbutton.y / ROUTINE_HEIGHT;
                    if (new_index >= 0 && new_index < routines->routine_count) {
                        return new_index;
                    }
                }
                break;
        }
    }
}

int select_routine_gui(RoutineList *routines) {
    int height = routines->routine_count * ROUTINE_HEIGHT;

    create_window(height);
    init_font_and_colors();

    int selected = handle_events(routines);

    XftDrawDestroy(xft_draw);
    XftFontClose(dpy, font);
    XFreeGC(dpy, gc);
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);

    return selected;
}
