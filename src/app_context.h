/*
 * Filename: app_context.h
 *
 * Description: Declarations for the AppContext structure, which holds global
 * X11-related resources and window parameters for the dock application.
 *
 * Author: Michael Knap
 * Date: 2025-02-13
 * License: MIT
 */

#ifndef APP_CONTEXT_H
#define APP_CONTEXT_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdint.h>

#define CLIPBOARD_BUFFER_SIZE 64

typedef struct {
    Display *display;
    Window window;
    GC gc;
    XVisualInfo vinfo;
    uint32_t dock_width;
    uint32_t dock_height;
    uint32_t rect_size;
} AppContext;

extern AppContext app;

#endif  // APP_CONTEXT_H
