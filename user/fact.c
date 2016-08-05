
#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	long n = strtol(argv[1],0,10);
	int acc=1;
	int i;
	for(i=1;i<=n;i++)
	{
		acc=acc*i;
	}
	cprintf("factorial(%d) = %d\n",n,acc);
	int temp = 0;
	for (i=0;i<1000000;i++){
		temp++;
	}
	exit();
}