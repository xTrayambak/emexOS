#include "boot.h"
#include "print.h"
#include <kernel/communication/serial.h>
#include <kernel/graph/graphics.h>

#include <string/string.h>
// all the print function will be migrated to this file
// because currently the console is a part of the screen (like console_scrollup)
// and this is bad thats why i create kernel processes and the bootscreen process
// which will handle all bootup messages and also the scroll up (and mb login)

bs_screen_t bs_screens[BS_MAX_SCREENS];
int bs_active = 0;

#define BS_BACKBUF_MAX (1920 * 1200)
static u32 bs_buffers[BS_MAX_SCREENS][BS_BACKBUF_MAX];
static u32 bs_pdw = 0; /* fb_pitch : 4 */
static u32 bs_fbh = 0; /* real fbh */

void bs_init_screens(void)
{
	bs_pdw = get_fb_pitch() / 4;
	bs_fbh = get_fb_height();
	for (int i = 0; i < BS_MAX_SCREENS; i++)
	{
		bs_screens[i].cursor_x = 0;
		bs_screens[i].cursor_y = 0;
		bs_screens[i].buffer = bs_buffers[i];
	    bs_screens[i].x = 0;
	    bs_screens[i].y = 0;
	    bs_screens[i].width = get_fb_width();
	    bs_screens[i].height = get_fb_height();
	}
}

void bs_switch(int id)
{
	//log("BS", "bootscreen switched", _d);
    if (id < 0 || id >= BS_MAX_SCREENS) return;
    int old = bs_active;
    bs_active = id;
    char buffer[64];
    buffer[0] = '\0';
    char num[16];
    str_append(buffer, "bootscreen switched; ");
    str_from_int(num, old);
    str_append(buffer, num);
    str_append(buffer, " to ");
    str_from_int(num, id);
    str_append(buffer, num);
    str_append(buffer, "\n");
    u32 *fb = get_framebuffer();
    if (!fb) return;
    memcpy(fb, bs_screens[id].buffer, bs_pdw * bs_fbh * sizeof(u32));
    log("[BOOTSCREEN]", buffer, _d);
    //print("\n", white());
}

void bs_set_region(int id, u32 x, u32 y, u32 w, u32 h)
{
    if (id < 0 || id >= BS_MAX_SCREENS) return;

    bs_screens[id].x = x;
    bs_screens[id].y = y;
    bs_screens[id].width = w;
    bs_screens[id].height = h;
}

bs_screen_t* bs_get_active(void)
{
	return &bs_screens[bs_active];
}

u32 *bs_backbuf_get(void)
{
	return bs_get_active()->buffer;
}

u32 bs_backbuf_pitch_dw(void)
{
	return bs_pdw;
}

u32 bs_backbuf_height(void)
{
	return bs_fbh;
}

static void bs_ensure_init(void)
{
	if (bs_pdw == 0) {
		bs_pdw = get_fb_pitch() / 4;
		bs_fbh = get_fb_height();
	}
}

void bs_flush_rows(u32 y, u32 row_count)
{
    u32 *fb  = get_framebuffer();
    u32 *buf = bs_get_active()->buffer;
    if (!fb || !bs_pdw) return;
    u32 end = y + row_count;
    if (end > bs_fbh) end = bs_fbh;
    if (end <= y) return;
    memcpy(
        fb  + y * bs_pdw,
        buf + y * bs_pdw,
        (end - y) * bs_pdw * sizeof(u32)
    );
}

/* from active backbuffer to framebuffer
 * row by wor
 */
void bs_flush_rect(u32 x, u32 y, u32 w, u32 h)
{
    u32 *fb  = get_framebuffer();
    u32 *buf = bs_get_active()->buffer;
    if (!fb || !bs_pdw) return;
    u32 end_y = y + h;
    if (end_y > bs_fbh) end_y = bs_fbh;
    u32 end_x = x + w;
    if (end_x > bs_pdw) end_x = bs_pdw;
    u32 copy_w = end_x - x;
    if (!copy_w) return;
    for (u32 row = y; row < end_y; row++) {
        memcpy(
            fb  + row * bs_pdw + x,
            buf + row * bs_pdw + x,
            copy_w * sizeof(u32)
        );
    }
}

/* entire backbuffer copy */
void bs_backbuf_flush_all(void)
{
	bs_ensure_init();
	u32 *fb = get_framebuffer();
	if (!fb || !bs_pdw || !bs_fbh) return;
	memcpy(
		fb, bs_get_active()->buffer,
		bs_pdw * bs_fbh * sizeof(u32)
	);
}

/* clears both backbuffer and framebuffer */
void bs_backbuf_clear(u32 color)
{
	bs_ensure_init();
	u32 *fb  = get_framebuffer();
	u32  len = bs_pdw * bs_fbh;
	if (!len) return;
	u32 *buf = bs_get_active()->buffer;

	if (color == 0) {
		memset(buf, 0, len * sizeof(u32));
	} else
	{
		u64 fill = ((u64)color << 32) | color;
		u64 *p64 = (u64 *)buf;
		u32 n64 = len / 2;
		for (u32 i = 0; i < n64; i++) p64[i] = fill;
		/* handle odd trailing pixel */
		if (len & 1) buf[len - 1] = color;
	}

	if (fb) memcpy(fb, buf, len * sizeof(u32));
}

static int bs_init_proc(kproc_t *self) {
    (void)self;
    bs_switch(BS1);
    log("[BOOTSCREEN]","init bootscreen\n", d);
    return KPROC_EFINE;
}

static int bs_tick(kproc_t *self) {
    (void)self;
    return KPROC_EFINE;
}

static void bs_fini(kproc_t *self) {
    (void)self;
}

kproc_t bootscreen_proc =
{
    .name     = "bootscreen",
    .state    = KPROC_STATE_READY,
    .flags    = KPROC_FLAG_EARLY | KPROC_FLAG_CRITICAL | KPROC_FLAG_PERMANENT,
    .priority = 128,
    .init     = bs_init_proc,
    .tick     = bs_tick,
    .fini     = bs_fini,
};

void init_bootscreen(void)
{
    bs_init_screens();

    u32 w = get_fb_width();
    u32 h = get_fb_height();

    bs_set_region(  BS1,
    	0,
     	0,
      	w / 2,
       	h
    );
    bs_set_region(  BS2,
        w / 2,
        0,
        w / 2,
        h
    );
    bs_set_region(  BS3,
    	0,
     	0,
      	w,
       	h
    );

    bs_switch(BS2);
    print("test", white());
    bs_switch(BS1);

    kproc_register_and_start(&bootscreen_proc);
}
