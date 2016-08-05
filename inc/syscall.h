#ifndef JOS_INC_SYSCALL_H
#define JOS_INC_SYSCALL_H

/* system call numbers */
enum {
	SYS_cputs = 0,
	SYS_cgetc,
	SYS_getenvid,
	SYS_env_destroy,
	SYS_page_alloc,
	SYS_page_map,
	SYS_page_unmap,
	SYS_exofork,
	SYS_env_set_status,
	SYS_env_set_pgfault_upcall,
	SYS_yield,
	SYS_ipc_try_send,
	SYS_ipc_recv,
	SYS_exec,
	SYS_wait,
	SYS_date,
	SYS_kill,
	SYS_trace_me,
	SYS_trace,
	SYS_get_child_tf,
	SYS_ptrace_attach,
	SYS_ptrace_peek,
	SYS_ptrace_poke,
	SYS_ptrace_set_debug_flag,
	SYS_ptrace_set_eip,
	SYS_oprof_me,
	NSYSCALLS
};

#endif /* !JOS_INC_SYSCALL_H */
