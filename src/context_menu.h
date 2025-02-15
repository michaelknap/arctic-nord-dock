/*
 * Filename: context_menu.h
 *
 * Description: Declarations for the context menu functionality, including
 * constants, color formatting, and menu layout definitions.
 *
 * Author: Michael Knap
 * Date: 2025-02-13
 * License: MIT
 */

#ifndef CONTEXT_MENU_H
#define CONTEXT_MENU_H

#include <stddef.h>
#include <stdint.h>

#include "color_box.h"

// Menu layout constants
#define MENU_ITEM_HEIGHT 20
#define MENU_ITEM_PADDING 5
#define MENU_WIDTH 80

// Context menu colors
#define LIGHT_GREY 0xCCCCCC
#define DARK_GREY 0x555555

// Supported output formats in order.
typedef enum {
    FORMAT_HTML_HEX,  // "#RRGGBB"
    FORMAT_RAW_HEX,   // "0xaabbcc"
    FORMAT_CSS_RGB,   // "rgb(R, G, B)"
    FORMAT_CSS_RGBA,  // "rgba(R, G, B, 1)"
    FORMAT_HSL,       // "hsl(H, S%, L%)"
    FORMAT_FLOAT,     // "0.54f, 0.22f, 0.44f"
    FORMAT_VEC3,      // "vec3(0.54f, 0.22f, 0.44f)"
    FORMAT_VEC4,      // "vec4(0.54f, 0.22f, 0.44f, 1.00f)"
    FORMAT_COUNT
} ColorFormat;

extern ColorFormat current_format;

void format_color(uint32_t color, ColorFormat format, char *buf,
                  size_t buf_size);

int context_menu_show(int x, int y);

#endif  // CONTEXT_MENU_H
