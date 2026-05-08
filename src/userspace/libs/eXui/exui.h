#pragma once

#include "../libfont/libfont.h"

#define EXUI_INPUTBOX_MAX 256

typedef struct
{
    char buf[EXUI_INPUTBOX_MAX];

    int 	len;
    int 	focused;
    int 	mask;
} exui_inputbox_t;

typedef struct
{
    void (*DrawRectangle)(int x, int y, int w, int h, unsigned int color);
    void (*DrawPixel) (int x, int y, unsigned int color);
    void (*DrawCursor) (int x, int y, unsigned int color, font_id_t font);

    void (*Print)(int x, int y, const char *text, unsigned int fg, unsigned int bg, font_id_t font);

    void (*Clear)(unsigned int color);
    void (*Flush)(void);

    int  (*InputBox)(int x, int y, int w, exui_inputbox_t *state, unsigned int fg, unsigned int bg, unsigned int border, font_id_t font, unsigned int keycode);
} eXui;

extern eXui exui; // exui.

void exui_init(int w, int h);
int exui_width(void);
int exui_height(void);