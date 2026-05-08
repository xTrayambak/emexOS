#include "exui.h"
#include "libdesktop.h"
#include "font8x12.h"
#include <stdlib.h>
#include <string.h>

static unsigned int *g_buf = NULL;
static int g_w  = 0;
static int g_h  = 0;

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

static int _fh(int size) { (void)size; return FONT8X12_H; }
static int _fw(int size) { (void)size; return FONT8X12_W; }

static void _draw_rectangle(int x, int y, int w, int h, unsigned int color)
{
    if (!g_buf) return;
    int dy, dx;
    for (dy = 0; dy < h; dy++)
    {
        int ay = y + dy;
        if (ay < 0 || ay >= g_h) continue;

        for (dx = 0; dx < w; dx++)
        {
            int ax = x + dx;
            if (ax < 0 || ax >= g_w) continue;
            g_buf[ay * g_w + ax] = color;
        }
    }
}

static void _draw_pixel(int x, int y, unsigned int color)
{
    if (!g_buf || x < 0 || x >= g_w || y < 0 || y >= g_h) return;
    g_buf[y * g_w + x] = color;
}

static void _print(int x, int y, const char *text, unsigned int fg, unsigned int bg, int size)
{
    if (!g_buf || !text) return;
    int px = x;
    int fh = _fh(size);
    int fw = _fw(size);
    while (*text)
    {
        unsigned char c = (unsigned char)*text & 0x7Fu;
        int row, col;
        for (row = 0; row < fh; row++)
        {
            int ay = y + row;
            if (ay < 0 || ay >= g_h) continue;
            unsigned int bits = font8x12_glyph(c, row);

            for (col = 0; col < fw; col++)
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

static void _clear(unsigned int color)
{
    if (!g_buf) return;
    int i, total = g_w * g_h;
    for (i = 0; i < total; i++) g_buf[i] = color;
}

static void _flush(void)
{
    if (!g_buf || g_w <= 0 || g_h <= 0) return;
    desktop.winbuf_write(g_buf, g_w, g_h);
}

eXui exui = {
    .draw_rectangle = _draw_rectangle,
    .draw_pixel     = _draw_pixel,
    .print          = _print,
    .clear          = _clear,
    .flush          = _flush,
};
