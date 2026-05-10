#ifndef DEVICE_PTY_H
#define DEVICE_PTY_H

#include <types.h>
#include <kernel/module/module.h>



#define PTY_MAX        8       /* number of pairs      */
#define PTY_BUF_SIZE   4096    /* ring buffer per side  */

typedef struct {
    /* slave → master  (child stdout/stderr → terminal emulator reads) */
    char    s2m_buf[PTY_BUF_SIZE];
    u32     s2m_head;       /* reader index  */
    u32     s2m_tail;       /* writer index  */
    u32     s2m_count;      /* bytes queued  */

    /* master → slave  (terminal emulator input → child stdin) */
    char    m2s_buf[PTY_BUF_SIZE];
    u32     m2s_head;
    u32     m2s_tail;
    u32     m2s_count;

    int     active;         /* 1 = allocated            */
    int     slave_open;     /* number of slave fds open */
    int     master_open;    /* master side open?        */
} pty_pair_t;




void pty_init(void);


int  pty_alloc(void);


void pty_free(int idx);


pty_pair_t *pty_get(int idx);




ssize_t pty_master_read (int idx, void *buf, size_t count);

ssize_t pty_master_write(int idx, const void *buf, size_t count);


ssize_t pty_slave_read  (int idx, void *buf, size_t count);

ssize_t pty_slave_write (int idx, const void *buf, size_t count);


extern driver_module ptmx_module;    /* /dev/ptmx    (master entry-point) */
extern driver_module pts0_module;    /* /dev/pts/0   (slave)              */




int pty_attach_slave_fds(int pty_idx);


int pty_open_master(int pty_idx);

#endif
