/*
 * Filename: color_box.h
 *
 * Description: Contains the declarations for the color box data structures and
 * functions used for drawing, interaction, and management of color boxes in the
 * dock.
 *
 * Author: Michael Knap
 * Date: 2025-02-13
 * License: MIT
 */

#ifndef COLOR_BOX_H
#define COLOR_BOX_H

#include <stdint.h>

// Color definitions from the Arctic Nord palette.
#define nord0 0x2E3440
#define nord1 0x3B4252
#define nord2 0x434C5E
#define nord3 0x4C566A
#define nord4 0xD8DEE9
#define nord5 0xE5E9F0
#define nord6 0xECEFF4
#define nord7 0x8FBCBB
#define nord8 0x88C0D0
#define nord9 0x81A1C1
#define nord10 0x5E81AC
#define nord11 0xBF616A
#define nord12 0xD08770
#define nord13 0xEBCB8B
#define nord14 0xA3BE8C
#define nord15 0xB48EAD

#define PALETTE_LENGTH 16

// Represents a single color box.
typedef struct ColorBox {
    uint32_t x;
    uint32_t y;
    uint32_t color;
    const char *label;
    bool is_clicked;
} ColorBox;

// Represents the dimensions of rendered text.
typedef struct {
    uint32_t width;
    uint32_t height;
} TextMetrics;

void initialize_color_boxes(void);

void draw_all_boxes(void);
void draw_colorbox(const ColorBox *box);

ColorBox *find_box(uint32_t x, uint32_t y);
bool is_point_inside_box(uint32_t x, uint32_t y, const ColorBox *box);
TextMetrics get_text_metrics(const char *text);

// Mouse interaction event handlers.
void colorbox_on_click(ColorBox *box);
void colorbox_on_release(ColorBox *box);

// Functions to manage the “last clicked” box.
void set_last_clicked_box(ColorBox *box);
ColorBox *get_last_clicked_box(void);
void clear_last_clicked_box(void);

void copy_color_from_box(const ColorBox *box);

#endif  // COLOR_BOX_H
