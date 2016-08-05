#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	// cprintf("came into date.c\n");
	struct rtcdate rd;

	sys_date(&rd);
	cprintf("date is %d/%d/%d\n",rd.day,rd.month,rd.year);
}