// shell program

#include <inc/lib.h>

#define WHITESPACE "\t\r "
#define ENTER 13
#define MAXARGS 11
#define ENV_PASTE3(x, y, z) x ## y ## z


int user_argc;
char *user_argv[MAXARGS];


int getcmd(char *buf, int nbuf){

	cprintf("$ ");
	memset(buf, 0, nbuf);

	char c = sys_cgetc();
	int i=0;
	while(c != ENTER && i < nbuf){
		if (c != 0){
			buf[i++] = c;
			const char * d = &c;
			sys_cputs(d, 1);
		}
		c = sys_cgetc();
	}
	buf[i] = 0;
	cprintf("\n");
	return 0;
}

int parsecmd(char * buf){
	
	int i;

	// Parse the command buffer into whitespace-separated arguments
	user_argc = 0;
	user_argv[user_argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (user_argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return -1;
		}
		user_argv[user_argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}

	user_argv[user_argc] = 0;
	return 0;
}


void
umain(int argc, char **argv)
{
	static char buf[100];

	// Read and run input commands.
	while(getcmd(buf, sizeof(buf)) == 0){		
		if (parsecmd(buf) == 0){
			if(strlen(buf)==0) // if user presses enter without anything else
				continue;
			if(strcmp(user_argv[user_argc-1],"&") == 0) {
				cprintf("Executing %s in background...\n",user_argv[0]);
				if(fork()==0) {
					sys_exec(user_argv[0],user_argv);
					exit();
				} else{
					continue;
				}
			} else {
				cprintf("Executing %s...\n",user_argv[0]);
				if(fork()==0) {
					sys_exec(user_argv[0],user_argv);
					exit();
				} else{
					sys_wait();
				}
			}
		}
	}
	exit();
}

