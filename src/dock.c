/*
 * Filename: dock.c
 *
 * Description: Implements the dock window creation, event processing, clipboard
 * support, and cleanup routines for the Arctic Nord Dock.
 *
 * Author: Michael Knap
 * Date: 2025-02-13
 * License: MIT
 */

#include "dock.h"

#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_context.h"
#include "color_box.h"
#include "context_menu.h"

// Global hints for the window manager (_MOTIF_WM_HINTS).
const Hints hints = { 2, 0, 0, 0, 0 };

// Increase the clipboard buffer size to accommodate longer strings.
static char clipboard_text[CLIPBOARD_BUFFER_SIZE] = {};

static Atom wm_delete;

int initialize_dock(Display *display, uint32_t dock_width,
                    uint32_t dock_height) {
    app.display     = display;
    app.dock_width  = dock_width;
    app.dock_height = dock_height;

    int screen = DefaultScreen(app.display);

    // Get visual information (we request a 24-bit TrueColor visual)
    if (!XMatchVisualInfo(app.display, screen, 24, TrueColor, &app.vinfo)) {
        (void)fprintf(stderr, "Failed to obtain matching visual info.\n");
        return -1;
    }

    // Create a simple window at the right side of the screen.
    app.window = XCreateSimpleWindow(
        app.display, DefaultRootWindow(app.display),
        DisplayWidth(app.display, screen) - app.dock_width,
        (int)((DisplayHeight(app.display, screen) - app.dock_height) / 2),
        app.dock_width, app.dock_height, 0, BlackPixel(app.display, screen),
        BlackPixel(app.display, screen));
    if (!app.window) {
        (void)fprintf(stderr, "Failed to create window.\n");
        return -1;
    }

    // Set window properties to remove decorations.
    Atom hints_atom = XInternAtom(app.display, "_MOTIF_WM_HINTS", False);
    XChangeProperty(app.display, app.window, hints_atom, hints_atom, 32,
                    PropModeReplace, (unsigned char *)&hints, 5);

    wm_delete = XInternAtom(app.display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(app.display, app.window, &wm_delete, 1);

    // Set the window name.
    const char *window_name = "Arctic Nord";
    XChangeProperty(app.display, app.window,
                    XInternAtom(app.display, "_NET_WM_NAME", False),
                    XInternAtom(app.display, "UTF8_STRING", False), 8,
                    PropModeReplace, (unsigned char *)window_name,
                    (int)strlen(window_name));

    // Set class hints.
    XClassHint *class_hint = XAllocClassHint();
    if (class_hint) {
        class_hint->res_name  = "arctic_nord";
        class_hint->res_class = "ArcticNordDock";
        XSetClassHint(app.display, app.window, class_hint);
        XFree(class_hint);
    }

    XSelectInput(app.display, app.window,
                 ExposureMask | ButtonPressMask | ButtonReleaseMask |
                     PointerMotionMask | PropertyChangeMask);

    XMapWindow(app.display, app.window);

    app.gc = XCreateGC(app.display, app.window, 0, nullptr);
    if (!app.gc) {
        (void)fprintf(stderr, "Failed to create graphics context.\n");
        XDestroyWindow(app.display, app.window);
        return -1;
    }

    // Reposition the window and set it always on top.
    XMoveWindow(
        app.display, app.window,
        DisplayWidth(app.display, screen) - app.dock_width,
        (int)(DisplayHeight(app.display, screen) - app.dock_height) / 2);
    set_above_state(app.display, app.window);

    return 0;
}

void handle_event(XEvent *event) {
    ColorBox *box = nullptr;
    switch (event->type) {
        case Expose:
            if (event->xexpose.count == 0) {
                draw_all_boxes();
            }
            break;

        case ButtonPress:
            if (event->xbutton.button == Button1) {
                // Left-click: copy the color using the global current_format.
                box = find_box(event->xbutton.x, event->xbutton.y);
                if (box) {
                    copy_color_from_box(box);
                    box->is_clicked = true;
                    draw_colorbox(box);
                    set_last_clicked_box(box);
                }
            } else if (event->xbutton.button == Button3) {
                // Right-click: show the context menu to change the global
                // format.
                box = find_box(event->xbutton.x, event->xbutton.y);
                if (box) {
                    Window child = 0;
                    int root_x   = 0;
                    int root_y   = 0;
                    XTranslateCoordinates(app.display, app.window,
                                          DefaultRootWindow(app.display),
                                          event->xbutton.x, event->xbutton.y,
                                          &root_x, &root_y, &child);
                    int chosen_format = context_menu_show(root_x, root_y);
                    if (chosen_format >= 0) {
                        current_format = (ColorFormat)chosen_format;
                        // Copy the color in the newly selected format
                        copy_color_from_box(box);
                    }
                }
            }
            break;

        case ButtonRelease:
            box = find_box(event->xbutton.x, event->xbutton.y);
            if (box && (get_last_clicked_box() == box)) {
                colorbox_on_release(box);
                draw_colorbox(box);
                clear_last_clicked_box();
            }
            break;

        case MotionNotify:
            box = find_box(event->xmotion.x, event->xmotion.y);
            if (get_last_clicked_box() && (get_last_clicked_box() != box)) {
                colorbox_on_release(get_last_clicked_box());
                draw_colorbox(get_last_clicked_box());
                clear_last_clicked_box();
            }
            break;

        case SelectionRequest:
            handle_selection_request(&event->xselectionrequest);
            break;

        case ClientMessage:
            if ((Atom)event->xclient.data.l[0] == wm_delete) {
                cleanup_dock();
                exit(EXIT_SUCCESS);  // NOLINT(concurrency-mt-unsafe)
            }
            break;

        default:
            break;
    }
}

void cleanup_dock(void) {
    if (app.gc) {
        XFreeGC(app.display, app.gc);
        app.gc = nullptr;
    }
    if (app.window) {
        XDestroyWindow(app.display, app.window);
        app.window = 0;
    }
    if (app.display) {
        XCloseDisplay(app.display);
        app.display = nullptr;
    }
}

void set_clipboard(const char *text) {
    if (!text) {
        return;
    }

    // Copy the provided text into our clipboard buffer.
    strncpy(clipboard_text, text, CLIPBOARD_BUFFER_SIZE - 1);
    clipboard_text[CLIPBOARD_BUFFER_SIZE - 1] = '\0';

    // Obtain atoms for the CLIPBOARD selection and UTF8_STRING.
    Atom clipboard   = XInternAtom(app.display, "CLIPBOARD", False);
    Atom utf8_string = XInternAtom(app.display, "UTF8_STRING", False);

    // Claim ownership of the CLIPBOARD.
    XSetSelectionOwner(app.display, clipboard, app.window, CurrentTime);
    if (XGetSelectionOwner(app.display, clipboard) != app.window) {
        (void)fprintf(stderr, "Failed to set clipboard owner\n");
        return;
    }

    // Update the window property to hold the clipboard text.
    XChangeProperty(app.display, app.window, utf8_string, utf8_string, 8,
                    PropModeReplace, (unsigned char *)clipboard_text,
                    (int)strlen(clipboard_text));
    XFlush(app.display);
}

void handle_selection_request(XSelectionRequestEvent *req) {
    XSelectionEvent notify;
    memset(&notify, 0, sizeof(notify));
    notify.type      = SelectionNotify;
    notify.requestor = req->requestor;
    notify.selection = req->selection;
    notify.target    = req->target;
    notify.property  = req->property;
    notify.time      = req->time;

    Atom utf8_string   = XInternAtom(app.display, "UTF8_STRING", False);
    Atom compound_text = XInternAtom(app.display, "COMPOUND_TEXT", False);
    Atom targets_atom  = XInternAtom(app.display, "TARGETS", False);

    if (req->target == targets_atom) {
        Atom supported_targets[] = { targets_atom, XA_STRING, utf8_string,
                                     compound_text };
        XChangeProperty(
            app.display, req->requestor, req->property, XA_ATOM, 32,
            PropModeReplace, (unsigned char *)supported_targets,
            sizeof(supported_targets) / sizeof(supported_targets[0]));
    } else if (req->target == XA_STRING || req->target == utf8_string ||
               req->target == compound_text) {
        XChangeProperty(app.display, req->requestor, req->property, req->target,
                        8, PropModeReplace, (unsigned char *)clipboard_text,
                        (int)strlen(clipboard_text));
    } else {
        notify.property = None;
    }
    XSendEvent(app.display, req->requestor, True, 0, (XEvent *)&notify);
}

void set_above_state(Display *display, Window window) {
    Atom net_wm_state = XInternAtom(display, "_NET_WM_STATE", False);
    Atom net_wm_state_above =
        XInternAtom(display, "_NET_WM_STATE_ABOVE", False);

    XClientMessageEvent xclient;
    memset(&xclient, 0, sizeof(xclient));
    xclient.type         = ClientMessage;
    xclient.window       = window;
    xclient.message_type = net_wm_state;
    xclient.format       = 32;
    xclient.data.l[0]    = 1;
    xclient.data.l[1]    = (long)net_wm_state_above;
    xclient.data.l[2]    = 0;
    xclient.data.l[3]    = 1;
    xclient.data.l[4]    = 0;

    XSendEvent(display, DefaultRootWindow(display), False,
               SubstructureRedirectMask | SubstructureNotifyMask,
               (XEvent *)&xclient);
}
