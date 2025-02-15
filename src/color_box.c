/*
 * Filename: color_box.c
 *
 * Description: Contains functions for drawing color boxes and handling mouse
 * interactions
 *
 * Author: Michael Knap
 * Date: 2025-02-13
 * License: MIT
 */

#include "color_box.h"

#include <X11/Xlib.h>
#include <stdint.h>
#include <string.h>

#include "app_context.h"
#include "context_menu.h"
#include "dock.h"

static ColorBox color_boxes[PALETTE_LENGTH] = {};

static ColorBox *last_clicked_box = nullptr;

void set_last_clicked_box(ColorBox *box) {
    last_clicked_box = box;
}

void clear_last_clicked_box(void) {
    last_clicked_box = nullptr;
}

ColorBox *get_last_clicked_box(void) {
    return last_clicked_box;
}

void copy_color_from_box(const ColorBox *box) {
    if (!box) {
        return;
    }
    char buffer[CLIPBOARD_BUFFER_SIZE];
    format_color(box->color, current_format, buffer, sizeof(buffer));
    set_clipboard(buffer);
}

void draw_colorbox(const ColorBox *box) {
    if (!box) {
        return;
    }

    uint32_t adjusted_rect_size = app.rect_size;
    uint32_t adjusted_x         = box->x;
    uint32_t adjusted_y         = box->y;

    // If the box is clicked, reduce its size to show a "pressed" effect.
    if (box->is_clicked) {
        adjusted_rect_size -= 5;
        adjusted_x += 2;
        adjusted_y += 2;
    }

    XClearArea(app.display, app.window, box->x, box->y, app.rect_size,
               app.rect_size, False);

    XSetForeground(app.display, app.gc, box->color);

    XFillRectangle(app.display, app.window, app.gc, adjusted_x, adjusted_y,
                   adjusted_rect_size, adjusted_rect_size);

    TextMetrics label_metrics  = get_text_metrics(box->label);
    uint32_t label_width       = label_metrics.width;
    uint32_t label_rect_width  = label_width + PADDING;
    uint32_t label_rect_height = label_metrics.height + PADDING;
    uint32_t label_rect_x      = adjusted_x;
    uint32_t label_rect_y = adjusted_y + adjusted_rect_size - label_rect_height;
    if (label_rect_y < adjusted_y) {
        label_rect_y      = adjusted_y;
        label_rect_height = adjusted_rect_size;
    }
    XSetForeground(app.display, app.gc, BACKGROUND);
    XFillRectangle(app.display, app.window, app.gc, label_rect_x, label_rect_y,
                   label_rect_width, label_rect_height);

    XSetForeground(app.display, app.gc, nord6);
    uint32_t text_x = label_rect_x + 2;
    uint32_t text_y =
        label_rect_y + ((label_rect_height + label_metrics.height) / 2);
    XDrawString(app.display, app.window, app.gc, text_x, text_y, box->label,
                (int)strlen(box->label));
}

void draw_all_boxes(void) {
    for (uint8_t i = 0; i < PALETTE_LENGTH;  // NOLINT(altera-unroll-loops)
         i++) {
        draw_colorbox(&color_boxes[i]);
    }
}

TextMetrics get_text_metrics(const char *text) {
    TextMetrics metrics = { 0, 0 };
    if (!text) {
        return metrics;
    }

    XFontStruct *font_info = XQueryFont(app.display, XGContextFromGC(app.gc));
    if (!font_info) {
        return metrics;
    }

    metrics.width  = XTextWidth(font_info, text, (int)strlen(text));
    metrics.height = font_info->ascent + font_info->descent;
    XFreeFontInfo(nullptr, font_info, 0);
    return metrics;
}

bool is_point_inside_box(uint32_t x, uint32_t y, const ColorBox *box) {
    if (!box) {
        return false;
    }
    return (bool)((x >= box->x) && (x <= (box->x + app.rect_size)) &&
                  (y >= box->y) && (y <= (box->y + app.rect_size)));
}

// Find which box (if any) contains the point (x, y).
ColorBox *find_box(uint32_t x, uint32_t y) {
    for (uint8_t i = 0; i < PALETTE_LENGTH;  // NOLINT(altera-unroll-loops)
         i++) {
        if (is_point_inside_box(x, y, &color_boxes[i])) {
            return &color_boxes[i];
        }
    }
    return nullptr;
}

void colorbox_on_release(ColorBox *box) {
    if (!box) {
        return;
    }
    box->is_clicked = false;
}

void initialize_color_boxes(void) {
    const uint32_t colors[PALETTE_LENGTH] = { nord0,  nord1,  nord2,  nord3,
                                              nord4,  nord5,  nord6,  nord7,
                                              nord8,  nord9,  nord10, nord11,
                                              nord12, nord13, nord14, nord15 };
    const char *labels[PALETTE_LENGTH]    = {
        "nord0",  "nord1",  "nord2",  "nord3", "nord4",  "nord5",
        "nord6",  "nord7",  "nord8",  "nord9", "nord10", "nord11",
        "nord12", "nord13", "nord14", "nord15"
    };

    const uint32_t box_y_start = PADDING;
    for (uint8_t i = 0; i < PALETTE_LENGTH;  // NOLINT(altera-unroll-loops)
         i++) {
        color_boxes[i].x          = PADDING;
        color_boxes[i].y          = box_y_start + i * (app.rect_size + PADDING);
        color_boxes[i].color      = colors[i];
        color_boxes[i].label      = labels[i];
        color_boxes[i].is_clicked = false;
    }
}
