
#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	long n = strtol(argv[1],0,10);

	int fib1=0;int fib2=1;
	int i,temp;
	for(i=1;i<n;i++)
	{
		temp = fib1;
		fib1 = fib2;
		fib2 = temp+fib2;
	}
	cprintf("fibonacci(%d) = %d\n",n,fib1);
}