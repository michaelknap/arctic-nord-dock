/*
 * Filename: dock.h
 *
 * Description: Declarations for the dock UI, including initialization, event
 * handling, clipboard support, and window management for the Arctic Nord Dock.
 *
 * Author: Michael Knap
 * Date: 2025-02-13
 * License: MIT
 */

#ifndef DOCK_H
#define DOCK_H

#include <X11/Xlib.h>
#include <stdint.h>

#include "app_context.h"

#define PADDING 5
#define DOCK_HEIGHT_MARGIN 0.20  // Dock hight is at 80% of the screen height
#define BACKGROUND 0x000000      // Black background for label rectangles

typedef struct {
    unsigned long flags;
    unsigned long functions;
    unsigned long decorations;
    long input_mode;
    unsigned long status;
} Hints;

extern const Hints hints;

// Initialize the dock window with the given dimensions.
// Returns 0 on success or a negative value on error.
int initialize_dock(Display *display, uint32_t dock_width,
                    uint32_t dock_height);

// Process an X11 event.
void handle_event(XEvent *event);

// Clean up and free resources allocated by the dock.
void cleanup_dock(void);

// Set the clipboard content (for use when a color box is clicked).
void set_clipboard(const char *text);

// Handle clipboard selection requests.
void handle_selection_request(XSelectionRequestEvent *req);

// Ask the window manager to keep the dock window above others.
void set_above_state(Display *display, Window window);

#endif  // DOCK_H
