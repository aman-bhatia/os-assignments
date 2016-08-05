// buggy program - causes a divide by zero exception

#include <inc/lib.h>


void
umain(int argc, char **argv)
{
	cprintf("\
--------------------------------------------------------------------------------\n\
To find factorial, type                   : fact [number]\n\
To find fibonacci, type(1-based indexing) : fib [number]\n\
To echo something, type                   : echo [something]\n\
To find date, type                        : date\n\
To kill a env, type                       : kill [envid(base 10)]\n\
To strace, type                           : strace [any of the above commands]\n\
Note : Put & at the end of any command if you want it to run in background\n\
--------------------------------------------------------------------------------\n");
}

