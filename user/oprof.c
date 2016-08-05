#include <inc/lib.h>

#define WHITESPACE "\t\r "
#define ENTER 10
#define MAXARGS 11
#define ENV_PASTE3(x, y, z) x ## y ## z


char *sca[20];
	
void initialize()
{
	sca[SYS_cputs]="cputs";
	sca[SYS_cgetc]="cgetc";
	sca[SYS_getenvid]="getenvid";
	sca[SYS_env_destroy]="env_destroy";
	sca[SYS_page_alloc]="page_alloc";
	sca[SYS_page_map]="page_map";
	sca[SYS_page_unmap]="page_unmap";
	sca[SYS_exofork]="exofork";
	sca[SYS_env_set_status]="env_set_status";
	sca[SYS_env_set_pgfault_upcall]="env_set_pgfault_upcall";
	sca[SYS_yield]="yield";
	sca[SYS_ipc_try_send]="ipc_try_send";
	sca[SYS_ipc_recv]="ipc_recv";
	sca[SYS_exec]="exec";
	sca[SYS_date]="date";
	sca[SYS_kill]="env_destroy_my";
	sca[SYS_wait]="wait";
	sca[SYS_trace_me]="trace_me";
	sca[SYS_trace]="trace";
	sca[SYS_get_child_tf]="get_child_tf";
}

void
umain(int argc, char **argv)
{
	initialize();

	envid_t fork_val = fork();
	if(fork_val==0) {
		sys_oprof_me();
		sys_exec(argv[1],argv+1);
	} else { 
		sys_wait();
		struct Trapframe tf;
		while(1) {
			int oprof_code = sys_trace(fork_val);
			if(oprof_code==SYS_env_destroy)
				break;

			sys_get_child_tf(fork_val,&tf);
			cprintf(" - System call made : %d\n",oprof_code);
			
			// int retval = tf.tf_regs.reg_eax;
			// cprintf(" - return value : %u\n",retval);

			// int arg1 = tf.tf_regs.reg_edx;
			// int arg2 = tf.tf_regs.reg_ecx;
			// int arg3 = tf.tf_regs.reg_ebx;
			// int arg4 = tf.tf_regs.reg_edi;
			// int arg5 = tf.tf_regs.reg_esi;
			// cprintf(" - arg1 : %u\n",arg1);
			// cprintf(" - arg2 : %u\n",arg2);
			// cprintf(" - arg3 : %u\n",arg3);
			// cprintf(" - arg4 : %u\n",arg4);
			// cprintf(" - arg5 : %u\n\n",arg5);
			cprintf("%u\n",tf.tf_eip);			
		}
	}
	exit();
}
