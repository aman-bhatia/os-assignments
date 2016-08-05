// shell program

#include <inc/lib.h>

#define WHITESPACE "\t\r "
#define ENTER 13
#define MAXARGS 11
#define ENV_PASTE3(x, y, z) x ## y ## z


int user_argc;
char *user_argv[MAXARGS];

void help(){
	cprintf("\
    ---------------------------------------------------\n\
                            USAGE\n\
    ---------------------------------------------------\n\
    - To run the program, type               : run\n\
    - To set breakpoint at address addr,type : b addr\n\
    - To single step the program, type       : si\n\
    - To continue program execution          : c\n\
    ---------------------------------------------------\n\n");
}

int getcmd(char *buf, int nbuf){

	cprintf("(jdb) ");
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
print_trapframe(struct Trapframe *tf)
{
	cprintf("--------------------\n");
	cprintf("     TRAP frame\n");
	cprintf("--------------------\n");
	cprintf("  edi  0x%08x\n", tf->tf_regs.reg_edi);
	cprintf("  esi  0x%08x\n", tf->tf_regs.reg_esi);
	cprintf("  ebp  0x%08x\n", tf->tf_regs.reg_ebp);
	cprintf("  oesp 0x%08x\n", tf->tf_regs.reg_oesp);
	cprintf("  ebx  0x%08x\n", tf->tf_regs.reg_ebx);
	cprintf("  edx  0x%08x\n", tf->tf_regs.reg_edx);
	cprintf("  ecx  0x%08x\n", tf->tf_regs.reg_ecx);
	cprintf("  eax  0x%08x\n", tf->tf_regs.reg_eax);

	cprintf("  es   0x----%04x\n", tf->tf_es);
	cprintf("  ds   0x----%04x\n", tf->tf_ds);
	
	cprintf("  eip  0x%08x\n", tf->tf_eip);
	cprintf("  cs   0x----%04x\n", tf->tf_cs);
	cprintf("  flag 0x%08x\n", tf->tf_eflags);
	if ((tf->tf_cs & 3) != 0) {
		cprintf("  esp  0x%08x\n", tf->tf_esp);
		cprintf("  ss   0x----%04x\n", tf->tf_ss);
	}
}


void
umain(int argc, char **argv)
{
	envid_t fork_val = fork();
	if(fork_val==0) {
		sys_ptrace_attach();
		sys_exec(argv[1],argv+1);
	} else { 
		sys_wait();
		cprintf("Attached process for '%s'\n",argv[1]);
		help();
		static char buf[100];
		// Read and run input commands.
		while(getcmd(buf, sizeof(buf)) == 0){		
			if (parsecmd(buf) == 0){
				if(strlen(buf)==0) // if user presses enter without anything else
					continue;
				
				if (strcmp(user_argv[0],"run") == 0){
					cprintf("User input = run\n");
					sys_env_set_status(fork_val, ENV_RUNNABLE);
					sys_yield();
				} else if (strcmp(user_argv[0],"b") == 0){
					if (user_argc != 2){
						help();
						continue;
					}

					int brkpoint_data;
					void* instr_addr = (void*)strtol(user_argv[1], NULL, 16);
					int int3 = 0xCC;
					sys_ptrace_peek(fork_val,instr_addr,(void*) &brkpoint_data);
					sys_ptrace_poke(fork_val,instr_addr,(void*) &int3);
					cprintf("Breakpoint set at address %08x\nRunning process execution...\n\n",instr_addr);

					sys_env_set_status(fork_val, ENV_RUNNABLE);
					sys_yield();

					cprintf("Breakpoint Found!\n");
					struct Trapframe tf;
					sys_get_child_tf(fork_val,&tf);
					print_trapframe(&tf);
					sys_ptrace_poke(fork_val,instr_addr,(void*) &brkpoint_data);
					sys_ptrace_set_eip(fork_val,instr_addr);

				} else if (strcmp(user_argv[0],"si") == 0){

					struct Trapframe tf;
					sys_get_child_tf(fork_val,&tf);
					void* instr_addr = (void*)(tf.tf_eip);
					int data;
					sys_ptrace_peek(fork_val,instr_addr,(void*) &data);
					cprintf("eip : %08x , next instruction : %08x\n",tf.tf_eip,data);
					
					sys_ptrace_set_debug_flag(fork_val,1);
					sys_env_set_status(fork_val, ENV_RUNNABLE);
					sys_yield();

				} else if (strcmp(user_argv[0],"c") == 0){
					
					sys_ptrace_set_debug_flag(fork_val,0);
					sys_env_set_status(fork_val, ENV_RUNNABLE);
					sys_yield();

				} else {
					help();
					continue;
				}
				// cprintf("Executing %s...\n",user_argv[0]);
				// if(fork()==0) {
				// 	sys_exec(user_argv[0],user_argv);
				// 	exit();
				// } else{
				// 	sys_wait();
				// }
			}
		}
		// struct Trapframe tf;
		// while(1) {
		// 	int syscall_code = sys_trace(fork_val);
		// 	if(syscall_code==SYS_env_destroy)
		// 		break;

		// 	sys_get_child_tf(fork_val,&tf);
		// 	cprintf(" - System call made : %s\n",sca[syscall_code]);
			
		// 	int retval = tf.tf_regs.reg_eax;
		// 	cprintf(" - return value : %u\n",retval);

		// 	int arg1 = tf.tf_regs.reg_edx;
		// 	int arg2 = tf.tf_regs.reg_ecx;
		// 	int arg3 = tf.tf_regs.reg_ebx;
		// 	int arg4 = tf.tf_regs.reg_edi;
		// 	int arg5 = tf.tf_regs.reg_esi;
		// 	cprintf(" - arg1 : %u\n",arg1);
		// 	cprintf(" - arg2 : %u\n",arg2);
		// 	cprintf(" - arg3 : %u\n",arg3);
		// 	cprintf(" - arg4 : %u\n",arg4);
		// 	cprintf(" - arg5 : %u\n\n",arg5);			
		// }
	}
	exit();
}

