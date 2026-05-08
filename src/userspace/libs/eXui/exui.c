#include "exui.h"
#include "libdesktop.h"
#include "libfont.h"
#include <stdlib.h>
#include <string.h>

static unsigned int *g_buf = NULL;
static int g_w = 0;
static int g_h = 0;

void exui_init(int w, int h)
{
    if (g_buf) free(g_buf);
    if (w <= 0 || h <= 0) return;

    g_buf = NULL;
    g_w = 0;
    g_h = 0;

    g_buf = (unsigned int *)malloc((unsigned)(w * h * 4));
    if (!g_buf) return;

    g_w = w;
    g_h = h;
    memset(g_buf, 0, (unsigned)(w * h * 4));
}

int exui_width(void)  { return g_w; }
int exui_height(void) { return g_h; }

static inline void _set(int x, int y, unsigned int c)
{
    if (!g_buf || x < 0 || x >= g_w || y < 0 || y >= g_h) return;
    g_buf[y * g_w + x] = c;
}

static void _DrawRectangle(int x, int y, int w, int h, unsigned int color)
{
    for (int dy = 0; dy < h; dy++)
        for (int dx = 0; dx < w; dx++)
            _set(x + dx, y + dy, color);
}

static void _DrawPixel(int x, int y, unsigned int color)
{
    _set(x, y, color);
}

static void _Print(int x, int y, const char *text, unsigned int fg, unsigned int bg, font_id_t font)
{
    if (!g_buf || !text) return;
    int fw = font_w(font);
    int fh = font_h(font);
    int px = x;
    while (*text)
    {
        unsigned char c = (unsigned char)*text & 0x7Fu;
        for (int row = 0; row < fh; row++)
        {
            int ay = y + row;
            if (ay < 0 || ay >= g_h) continue;
            uint16_t bits = font_glyph(font, c, row);
            for (int col = 0; col < fw; col++)
            {
                int ax = px + col;
                if (ax < 0 || ax >= g_w) continue;
                g_buf[ay * g_w + ax] = (bits & (1u << col)) ? fg : bg;
            }
        }
        text++;
        px += fw;
    }
}

static void _DrawCursor(int x, int y, unsigned int color, font_id_t font)
{
    _DrawRectangle(x, y, 2, font_h(font), color);
}

static int _InputBox(int x, int y, int w, exui_inputbox_t *state, unsigned int fg, unsigned int bg, unsigned int border, font_id_t font, unsigned int keycode)
{
    if (!state) return 0;
    int fw = font_w(font);
    int fh = font_h(font);
    int h = fh + 6;
    int committed = 0;

    if (keycode != 0)
    {
        char c = (char)(keycode & 0xFF);
        if (keycode == '\n' || keycode == '\r')
        {
            committed = 1;
        } else if (keycode == '\b')
        {
            if (state->len > 0) state->buf[--state->len] = '\0';
        } else if (c >= 0x20 && c <= 0x7E)
        {
            if (state->len < EXUI_INPUTBOX_MAX - 1)
            {
                state->buf[state->len++] = c;
                state->buf[state->len]   = '\0';
            }
        }
    }

    _DrawRectangle(x, y, w, h, bg);
    _DrawRectangle(x, y,  w, 1, border);
    _DrawRectangle(x, y + h - 1, w, 1, border);
    _DrawRectangle(x, y, 1, h, border);
    _DrawRectangle(x + w - 1, y, 1, h, border);

    if (state->len > 0)
    {
        char display[EXUI_INPUTBOX_MAX];
        if (state->mask)
        {
            for (int i = 0; i < state->len; i++) display[i] = '*';
            display[state->len] = '\0';
        } else {
            for (int i = 0; i <= state->len; i++) display[i] = state->buf[i];
        }
        int max_chars = (w - 6) / (fw > 0 ? fw : 1);
        int start = state->len > max_chars ? state->len - max_chars : 0;
        _Print(x + 3, y + 3, display + start, fg, bg, font);
    }

    if (state->focused)
    {
        int max_chars = (w - 6) / (fw > 0 ? fw : 1);
        int clipped   = state->len > max_chars ? max_chars : state->len;
        _DrawCursor(x + 3 + clipped * fw, y + 3, fg, font);
    }

    return committed;
}

static void _Clear(unsigned int color)
{
    if (!g_buf) return;
    int total = g_w * g_h;
    for (int i = 0; i < total; i++) g_buf[i] = color;
}

static void _Flush(void)
{
    if (!g_buf || g_w <= 0 || g_h <= 0) return;
    desktop.winbuf_write(g_buf, g_w, g_h);
}

eXui exui = {
    .DrawRectangle = _DrawRectangle,
    .DrawPixel     = _DrawPixel,
    .Print         = _Print,
    .DrawCursor    = _DrawCursor,
    .InputBox      = _InputBox,
    .Clear         = _Clear,
    .Flush         = _Flush,
};