#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	int i;
	for(i=1;argv[i];i++)
	{
		cprintf("%s ",argv[i]);
	}
	cprintf("\n");
}