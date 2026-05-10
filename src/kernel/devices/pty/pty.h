#ifndef DEVICE_PTY_H
#define DEVICE_PTY_H

#include <types.h>
#include <kernel/module/module.h>

/*
 * PTY – pseudo-terminal pair
 *
 *   master  ←→  ring buffer  ←→  slave
 *
 * The master side is used by the terminal emulator (e.g. shelly).
 * The slave  side is given to the child process as fd 0/1/2.
 *
 * Data flow:
 *   child writes to slave  →  data appears in m2s_buf  →  master reads it
 *   master writes           →  data appears in s2m_buf  →  slave/child reads it
 */

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

/* ---- kernel API (called from syscalls / VFS) ---- */

/* Initialise the PTY subsystem – call once at boot */
void pty_init(void);

/* Allocate a new PTY pair. Returns index (0..PTY_MAX-1) or -1 */
int  pty_alloc(void);

/* Free a PTY pair */
void pty_free(int idx);

/* Get the raw pair struct (for ioctl etc.) */
pty_pair_t *pty_get(int idx);

/* ---- data path ---- */

/* Master side: read what the slave wrote (child stdout) */
ssize_t pty_master_read (int idx, void *buf, size_t count);
/* Master side: inject input for the slave (child stdin) */
ssize_t pty_master_write(int idx, const void *buf, size_t count);

/* Slave side:  read input from master (child stdin) */
ssize_t pty_slave_read  (int idx, void *buf, size_t count);
/* Slave side:  write output (child stdout) → master will read it */
ssize_t pty_slave_write (int idx, const void *buf, size_t count);

/* ---- devfs modules ---- */
extern driver_module ptmx_module;    /* /dev/ptmx    (master entry-point) */
extern driver_module pts0_module;    /* /dev/pts/0   (slave)              */

/* ---- helper for process setup ---- */

/*
 * Wire a process's fd 0, 1, 2 to a PTY slave.
 * Call this from execve/fork BEFORE jumping to userspace.
 * Returns 0 on success, -1 on error.
 */
int pty_attach_slave_fds(int pty_idx);

/*
 * Open master side and return its fd.  Returns fd >= 3 or -1 on error.
 */
int pty_open_master(int pty_idx);

#endif
