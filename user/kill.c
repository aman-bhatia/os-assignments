
#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	envid_t proc = (envid_t)strtol(argv[1],0,10);

	cprintf("killing env %08x\n",proc);
	sys_kill(proc);
}