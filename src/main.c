/*
 * Filename: main.c
 *
 * Description: Main entry point for the Arctic Nord Dock application.
 * Initializes the X11 display and dock, and runs the main event loop.
 *
 * Author: Michael Knap
 * Date: 2025-02-13
 * License: MIT
 */

#include <X11/Xlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "app_context.h"
#include "color_box.h"
#include "dock.h"

int main(void) {
    Display *display = XOpenDisplay(nullptr);
    if (!display) {
        (void)fprintf(stderr, "Unable to open display.\n");
        return EXIT_FAILURE;
    }

    int screen              = DefaultScreen(display);
    uint32_t display_height = DisplayHeight(display, screen);

    // Calculate the size of each color box and the dock dimensions.
    app.rect_size =
        (uint32_t)((display_height - (DOCK_HEIGHT_MARGIN * display_height)) /
                   PALETTE_LENGTH);
    app.dock_width  = 2 * PADDING + app.rect_size;
    app.dock_height = PALETTE_LENGTH * (app.rect_size + PADDING) + PADDING;

    // Initialize the dock window.
    if (initialize_dock(display, app.dock_width, app.dock_height) != 0) {
        XCloseDisplay(display);
        return EXIT_FAILURE;
    }

    initialize_color_boxes();

    // This loop blocks waiting for the next X event.
    // Termination occurs only when the WM_DELETE_WINDOW event is received.
    XEvent event;
    while (1) {  // NOLINT(altera-unroll-loops)
        XNextEvent(display, &event);
        handle_event(&event);
    }

    cleanup_dock();
    return EXIT_SUCCESS;
}
