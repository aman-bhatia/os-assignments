// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/trap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "backtrace", "Backtraces the function call", mon_backtrace },
	{ "si", "Single step execution", mon_si },
	{ "c", "Continue Execution", mon_c },
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < NCOMMANDS; i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int dummy_function_for_current_eip(){
	int ebp = read_ebp();
	int eip = *((int*)ebp + 1);
	return eip;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// Your code here.

	int eip, ebp, arg1, arg2, arg3, arg4, arg5;
	struct Eipdebuginfo info;

	eip = dummy_function_for_current_eip();

	cprintf("Stack backtrace:\n");
	cprintf("    current eip=%08x\n",eip);

	if( debuginfo_eip(eip,&info) == 0){
		cprintf("    \t%s:%d: %.*s+%d\n",info.eip_file, info.eip_line, info.eip_fn_namelen, info.eip_fn_name, (eip-info.eip_fn_addr));
	} else {
		cprintf("Address passed to debuginfo_eip not found\n");
	}

	ebp = read_ebp();


	while (ebp != 0){
		eip  = *((int*)ebp + 1);
		arg1 = *((int*)ebp + 2);
		arg2 = *((int*)ebp + 3);
		arg3 = *((int*)ebp + 4);
		arg4 = *((int*)ebp + 5);
		arg5 = *((int*)ebp + 6);

		cprintf("    ebp %08x  eip %08x  args %08x %08x %08x %08x %08x\n",ebp, eip, arg1, arg2, arg3, arg4, arg5);
		if( debuginfo_eip(eip,&info) == 0){
			cprintf("    \t%s:%d: %.*s+%d\n",info.eip_file, info.eip_line, info.eip_fn_namelen, info.eip_fn_name, (eip-info.eip_fn_addr));
		} else {
			cprintf("Address passed to debuginfo_eip not found\n");
		}

		ebp = *(int*)ebp;
	}
	
	return 0;
}

// LAB 3 single step and continue functions //

int mon_si(int argc, char **argv, struct Trapframe *tf){
	tf->tf_eflags |= FL_TF;
	return -1;
}

int mon_c(int argc, char **argv, struct Trapframe *tf){
	tf->tf_eflags &= (~(FL_TF));
	return -1;
}


/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < NCOMMANDS; i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	if (tf != NULL)
		print_trapframe(tf);

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
