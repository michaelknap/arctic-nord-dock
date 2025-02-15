/*
 * Filename: context_menu.c
 *
 * Description: Implements the context menu for selecting the color format. This
 * includes drawing the menu and handling user interactions.
 *
 * Author: Michael Knap
 * Date: 2025-02-13
 * License: MIT
 */

#include "context_menu.h"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_context.h"

// Menu item labels corresponding to the ColorFormat enum.
static const char *menu_items[FORMAT_COUNT] = {
    "HTML HEX", "Raw HEX", "CSS RGB", "CSS RGBA", "HSL", "Float", "Vec3", "Vec4"
};

ColorFormat current_format = FORMAT_HTML_HEX;

void format_color(uint32_t color, ColorFormat format, char *buf,
                  size_t buf_size) {
    unsigned int r = (color >> 16U) & 0xFFU;
    unsigned int g = (color >> 8U) & 0xFFU;
    unsigned int b = color & 0xFFU;
    double r_norm  = r / 255.0;
    double g_norm  = g / 255.0;
    double b_norm  = b / 255.0;

    switch (format) {
        case FORMAT_HTML_HEX:
            (void)snprintf(buf, buf_size, "#%02X%02X%02X", r, g, b);
            break;
        case FORMAT_RAW_HEX:
            (void)snprintf(buf, buf_size, "0x%02x%02x%02x", r, g, b);
            break;
        case FORMAT_CSS_RGB:
            (void)snprintf(buf, buf_size, "rgb(%u, %u, %u);", r, g, b);
            break;
        case FORMAT_CSS_RGBA:
            (void)snprintf(buf, buf_size, "rgba(%u, %u, %u, 1);", r, g, b);
            break;
        case FORMAT_HSL: {
            double max   = fmax(r_norm, fmax(g_norm, b_norm));
            double min   = fmin(r_norm, fmin(g_norm, b_norm));
            double delta = max - min;
            double h     = 0.0;
            double s     = 0.0;
            double l     = (max + min) / 2.0;

            if (delta != 0.0) {
                s = (l < 0.5) ? delta / (max + min) : delta / (2.0 - max - min);
                if (max == r_norm) {
                    h = (g_norm - b_norm) / delta;
                } else if (max == g_norm) {
                    h = 2.0 + (b_norm - r_norm) / delta;
                } else {  // max == b_norm
                    h = 4.0 + (r_norm - g_norm) / delta;
                }
                h *= 60.0;
                if (h < 0) {
                    h += 360.0;
                }
            }
            (void)snprintf(buf, buf_size, "hsl(%d, %d%%, %d%%);", (int)round(h),
                           (int)round(s * 100), (int)round(l * 100));
            break;
        }
        case FORMAT_FLOAT:
            (void)snprintf(buf, buf_size, "%.2ff, %.2ff, %.2ff", r_norm, g_norm,
                           b_norm);
            break;
        case FORMAT_VEC3:
            (void)snprintf(buf, buf_size, "vec3(%.2ff, %.2ff, %.2ff)", r_norm,
                           g_norm, b_norm);
            break;
        case FORMAT_VEC4:
            (void)snprintf(buf, buf_size, "vec4(%.2ff, %.2ff, %.2ff, 1.00f)",
                           r_norm, g_norm, b_norm);
            break;
        default:
            buf[0] = '\0';
            break;
    }
}

static void draw_context_menu(Display *dpy, Window menu_win, int screen,
                              int hover_item) {
    GC gc = XCreateGC(dpy, menu_win, 0, nullptr);

    XSetForeground(dpy, gc, BlackPixel(dpy, screen));
    XFillRectangle(dpy, menu_win, gc, 0, 0, MENU_WIDTH,
                   MENU_ITEM_HEIGHT * FORMAT_COUNT);

    for (int i = 0; i < FORMAT_COUNT; i++) {  // NOLINT(altera-unroll-loops)
        int item_y = i * MENU_ITEM_HEIGHT;

        if (i == hover_item) {
            XSetForeground(dpy, gc, LIGHT_GREY);
            XFillRectangle(dpy, menu_win, gc, 0, item_y, MENU_WIDTH,
                           MENU_ITEM_HEIGHT);
            XSetForeground(dpy, gc, BlackPixel(dpy, screen));
        } else if (i == (int)current_format) {
            XSetForeground(dpy, gc, DARK_GREY);
            XFillRectangle(dpy, menu_win, gc, 0, item_y, MENU_WIDTH,
                           MENU_ITEM_HEIGHT);
            XSetForeground(dpy, gc, WhitePixel(dpy, screen));
        } else {
            XSetForeground(dpy, gc, WhitePixel(dpy, screen));
        }

        XDrawString(dpy, menu_win, gc, MENU_ITEM_PADDING,
                    item_y + MENU_ITEM_HEIGHT - MENU_ITEM_PADDING,
                    menu_items[i], (int)strlen(menu_items[i]));
    }

    XFreeGC(dpy, gc);
}

int context_menu_show(int x, int y) {
    Display *dpy    = app.display;
    int screen      = DefaultScreen(dpy);
    int menu_height = MENU_ITEM_HEIGHT * FORMAT_COUNT;

    int screen_width = DisplayWidth(dpy, screen);
    if (x + MENU_WIDTH > screen_width) {
        x = screen_width - MENU_WIDTH - (int)app.dock_width;
    }

    int screen_height = DisplayHeight(dpy, screen);
    if (y + menu_height >
        (int)(app.dock_height + ((screen_height - app.dock_height) / 2))) {
        y = (int)(app.dock_height + ((screen_height - app.dock_height) / 2) -
                  menu_height);
    }

    XSetWindowAttributes attrs;
    attrs.override_redirect = True;
    attrs.background_pixel  = BlackPixel(dpy, screen);

    Window menu_win =
        XCreateWindow(dpy, DefaultRootWindow(dpy), x, y, MENU_WIDTH,
                      menu_height, 1, CopyFromParent, InputOutput,
                      CopyFromParent, CWOverrideRedirect | CWBackPixel, &attrs);

    XSelectInput(
        dpy, menu_win,
        ExposureMask | ButtonPressMask | PointerMotionMask | LeaveWindowMask);

    XMapRaised(dpy, menu_win);
    XFlush(dpy);

    int selected_item = -1;
    int hover_item    = -1;
    bool done         = false;

    draw_context_menu(dpy, menu_win, screen, hover_item);

    while (!done) {  // NOLINT(altera-unroll-loops)
        XEvent ev;
        XNextEvent(dpy, &ev);

        // If event is outside our menu, close it
        if (ev.xany.window != menu_win) {
            if (ev.type == ButtonPress) {
                done = true;
            }
            continue;
        }

        switch (ev.type) {
            case Expose: {
                draw_context_menu(dpy, menu_win, screen, hover_item);
                break;
            }

            case MotionNotify: {
                int new_hover = ev.xmotion.y / MENU_ITEM_HEIGHT;
                if (new_hover < 0 || new_hover >= FORMAT_COUNT) {
                    new_hover = -1;
                }
                if (new_hover != hover_item) {
                    hover_item = new_hover;
                    draw_context_menu(dpy, menu_win, screen, hover_item);
                }
                break;
            }
            case LeaveNotify: {
                // When the pointer leaves the menu window, reset the hover.
                if (hover_item != -1) {
                    hover_item = -1;
                    draw_context_menu(dpy, menu_win, screen, hover_item);
                }
                break;
            }
            case ButtonPress: {
                int click_y = ev.xbutton.y;
                if (click_y < 0 || click_y > menu_height) {
                    done = true;  // Click outside -> close menu
                } else {
                    selected_item = click_y / MENU_ITEM_HEIGHT;
                    done          = true;
                }
                break;
            }

            default:
                break;
        }
    }

    // Close and destroy the menu
    XUnmapWindow(dpy, menu_win);
    XDestroyWindow(dpy, menu_win);
    return selected_item;
}
