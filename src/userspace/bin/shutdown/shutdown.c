#include <stdio.h>
#include <emx/system.h>

int main()
{
	// TODO: Accept an optional delay argument in hh:mm format and sleep until that many seconds before exec'ing the syscall.
	
	int ret = reboot(RSYSTEM_CMD_POWEROFF);
	if (ret < 0)
		perror("reboot");

	return ret;
}