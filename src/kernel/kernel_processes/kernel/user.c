#include "gen.h"
#include "../shells/shells.h"
#include "../bootscreen/boot.h"

#include <kernel/include/assembly.h>
#include <kernel/packages/emex/emex.h>

#include <kernel/mem/memlog.h>

#include <kernel/devices/pty/pty.h>
#include <kernel/file_systems/vfs/vfs.h>

// path to the init app in the VFS (loaded from initrd.cpio)
#define SYSTEMLOCATE "/emr/system/system.emx"
//mod
//#define LOGINLOCATE "/emr/bin/login.elf"

/*
 * Wire fd 0/1/2:
 *   fd 0 (stdin)  → /dev/tty0   (keyboard input, for now)
 *   fd 1 (stdout) → /dev/pts/0  (PTY slave — output goes to ring buffer)
 *   fd 2 (stderr) → /dev/pts/0  (PTY slave — same)
 *
 * The GUI shell (shelly) opens /dev/ptmx to read child output from master.
 */
static void setup_stdio(void)
{
    /* 1) initialise PTY and allocate pair 0 */
    pty_init();
    int pty_idx = pty_alloc();
    if (pty_idx < 0) {
        log("[INIT]", "WARN: pty_alloc failed\n", warning);
    }

    /* 2) stdin fd 0 → /dev/tty0 (keyboard reads still work) */
    int tty_fd = fs_open("/dev/tty0", O_RDWR);
    if (tty_fd >= 0) {
        fs_file *f = fs_get_file(tty_fd);
        if (f) fs_set_fd(0, f); /* stdin */
    }

    /* 3) stdout/stderr fd 1,2 → /dev/pts/0 (PTY slave) */
    int pts_fd = fs_open("/dev/pts/0", O_RDWR);
    if (pts_fd >= 0) {
        fs_file *f = fs_get_file(pts_fd);
        if (f) {
            fs_set_fd(1, f);   /* stdout → PTY slave */
            fs_set_fd(2, f);   /* stderr → PTY slave */
        }
        log("[INIT]", "stdio: fd0->tty0, fd1/2->pts/0\n", d);
    } else {
        /* fallback: wire stdout/stderr to tty0 if PTY not ready */
        if (tty_fd >= 0) {
            fs_file *f = fs_get_file(tty_fd);
            if (f) {
                fs_set_fd(1, f);
                fs_set_fd(2, f);
            }
        }
        log("[INIT]", "stdio: fd0/1/2->tty0 (PTY fallback)\n", warning);
    }
}

void uproc(void) {
    #if ENABLE_ULIME
        if (!proc_mgr || !ulime) {
            log("[INIT]", "proc_mgr or ulime not ready\n", error);
            return;
        }

        /* ---- set up stdio BEFORE launching userspace ---- */
        setup_stdio();

        log("[INIT]", "loading " SYSTEMLOCATE "...\n", d);

        ulime_proc_t *init_proc = NULL;
        int result = emex_launch_app(SYSTEMLOCATE, &init_proc);

        if (result != EMEX_OK || !init_proc) {
            log("[INIT]", "failed to load " SYSTEMLOCATE "\n", error);
            log("[INIT]", "system cannot continue without init\n", error);
            //while (1) __asm__ volatile("cli; hlt");
            // hcf();
            return;
        }

        log("[INIT]", "jumping to userspace\n", success);

        //memlog_print_map();

        //first clear screen
        clear(0xff000000);
        bs_switch(USER_SCREEN_MODE);

        dump_kprocesses();
        proc_list_procs(proc_mgr);
        ulime->ptr_proc_curr = init_proc;

        //hcf();

        log("[INIT]", "jumping to init_proc\n", d);

        //clear(0xff000000);

        #if JUMPTOUSER == 1

        	if (mt) {
	            mt_add_task(mt, init_proc); // register init with mt
	            mt_start(mt); // and launch
				//never returns
	        }
            JumpToUserspace(init_proc); // fallback if mt is not up

        #endif

        // if JumpToUserspace fails
        emergency_shell();

        // if emergency_shell fails
        recovery_shell();

        // if recovery_shell fails
        __builtin_unreachable();

        //proc_list_procs(proc_mgr);
    #endif
}
