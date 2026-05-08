#pragma once

#define EXUI_SIZE_12 12

typedef struct {
    void (*draw_rectangle)(int x, int y, int w, int h, unsigned int color);
    void (*draw_pixel)(int x, int y, unsigned int color);
    void (*print)(int x, int y, const char *text, unsigned int fg, unsigned int bg, int size);
    void (*clear)(unsigned int color);
    void (*flush)(void);
} eXui;

extern eXui exui; // exui.

void exui_init(int w, int h);
int exui_width(void);
int exui_height(void);
