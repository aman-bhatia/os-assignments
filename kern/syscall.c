/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/syscall.h>
#include <kern/console.h>
#include <kern/sched.h>


// Print a string to the system console.
// The string is exactly 'len' characters long.
// Destroys the environment on memory errors.
static void
sys_cputs(const char *s, size_t len)
{
	// Check that the user has permission to read memory [s, s+len).
	// Destroy the environment if not.

	// LAB 3: Your code here.
	user_mem_assert(curenv, s, len, PTE_P | PTE_U);

	// Print the string supplied by the user.
	cprintf("%.*s", len, s);
}

// Read a character from the system console without blocking.
// Returns the character, or 0 if there is no input waiting.
static int
sys_cgetc(void)
{
	return cons_getc();
}

// Returns the current environment's envid.
static envid_t
sys_getenvid(void)
{
	return curenv->env_id;
}

// Destroy a given environment (possibly the currently running environment).
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_destroy(envid_t envid)
{
	int r;
	struct Env *e;

	if ((r = envid2env(envid, &e, 1)) < 0)
		return r;
	if (e == curenv){
		cprintf("[%08x] exiting gracefully\n", curenv->env_id);
		if(curenv->env_being_oprofed==1) {
			struct Env *parentEnv;
			int r;
			if ((r = envid2env(curenv->env_parent_id, &parentEnv, 0)) != 0){
				panic("Error in getting parent env id in syscall in trap trap_dispatch : %e",r);
			}
			parentEnv->env_tf.tf_regs.reg_eax = SYS_env_destroy;
			parentEnv->env_status = ENV_RUNNABLE;
			curenv->env_status = ENV_NOT_RUNNABLE;
		}
	} else
		cprintf("[%08x] destroying %08x\n", curenv->env_id, e->env_id);
	env_destroy(e);
	return 0;
}

// Deschedule current environment and pick a different one to run.
static void
sys_yield(void)
{
	sched_yield();
}

// this new process to make the parrent not runnable so that the child can work in foreground
//in env_destroy make suitable change to make parent runnable again.

static void sys_wait(void)
{
	curenv->env_status = ENV_NOT_RUNNABLE;
}


// Allocate a new environment.
// Returns envid of new environment, or < 0 on error.  Errors are:
//	-E_NO_FREE_ENV if no free environment is available.
//	-E_NO_MEM on memory exhaustion.
static envid_t
sys_exofork(void)
{
	// Create the new environment with env_alloc(), from kern/env.c.
	// It should be left as env_alloc created it, except that
	// status is set to ENV_NOT_RUNNABLE, and the register set is copied
	// from the current environment -- but tweaked so sys_exofork
	// will appear to return 0.

	// LAB 4: Your code here.
	// panic("sys_exofork not implemented");

	struct Env * e;
	int r;
	if ((r = env_alloc(&e,thiscpu->cpu_env->env_id)) < 0){
		cprintf("Error in sys_exofork() : %e\n", r);
		return -E_NO_FREE_ENV;
	}

	e->env_status = ENV_NOT_RUNNABLE;
	e->env_tf = thiscpu->cpu_env->env_tf;
	e->env_tf.tf_regs.reg_eax = 0;
	
	return e->env_id;

}

// Set envid's env_status to status, which must be ENV_RUNNABLE
// or ENV_NOT_RUNNABLE.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if status is not a valid status for an environment.
static int
sys_env_set_status(envid_t envid, int status)
{
	// Hint: Use the 'envid2env' function from kern/env.c to translate an
	// envid to a struct Env.
	// You should set envid2env's third argument to 1, which will
	// check whether the current environment has permission to set
	// envid's status.

	// LAB 4: Your code here.
	// panic("sys_env_set_status not implemented");

	if (!(status == ENV_RUNNABLE || status != ENV_NOT_RUNNABLE))
		return -E_INVAL;

	struct Env * e;
	if ( envid2env(envid,&e,1) < 0)
		return -E_BAD_ENV;

	e->env_status = status;
	return 0;
}

// Set the page fault upcall for 'envid' by modifying the corresponding struct
// Env's 'env_pgfault_upcall' field.  When 'envid' causes a page fault, the
// kernel will push a fault record onto the exception stack, then branch to
// 'func'.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_set_pgfault_upcall(envid_t envid, void *func)
{
	// LAB 4: Your code here.
	// panic("sys_env_set_pgfault_upcall not implemented");

	struct Env *e;

	int r;
	if ((r = envid2env(envid, &e, 1)) < 0)
		return -E_BAD_ENV;

	e->env_pgfault_upcall = func;
	return 0;

}

// Allocate a page of memory and map it at 'va' with permission
// 'perm' in the address space of 'envid'.
// The page's contents are set to 0.
// If a page is already mapped at 'va', that page is unmapped as a
// side effect.
//
// perm -- PTE_U | PTE_P must be set, PTE_AVAIL | PTE_W may or may not be set,
//         but no other bits may be set.  See PTE_SYSCALL in inc/mmu.h.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if va >= UTOP, or va is not page-aligned.
//	-E_INVAL if perm is inappropriate (see above).
//	-E_NO_MEM if there's no memory to allocate the new page,
//		or to allocate any necessary page tables.
static int
sys_page_alloc(envid_t envid, void *va, int perm)
{
	// Hint: This function is a wrapper around page_alloc() and
	//   page_insert() from kern/pmap.c.
	//   Most of the new code you write should be to check the
	//   parameters for correctness.
	//   If page_insert() fails, remember to free the page you
	//   allocated!

	// LAB 4: Your code here.
	// panic("sys_page_alloc not implemented");

	struct Env * e;
	int r;
	if ((r = envid2env(envid, &e, 1)) < 0)
		return -E_BAD_ENV;

	if ((uint32_t) va >= UTOP || (uint32_t) va % PGSIZE != 0)
		return -E_INVAL;

	if ((perm | PTE_P | PTE_U) != perm || (perm & ~PTE_SYSCALL) != 0)
		return -E_INVAL;


	struct PageInfo * p;
	if ((p = page_alloc(ALLOC_ZERO)) == NULL)
		return -E_NO_MEM;

	if ((r = page_insert(e->env_pgdir, p, va, perm)) < 0) {
		page_free(p);
		return -E_NO_MEM;
	}

	return 0;
}

// Map the page of memory at 'srcva' in srcenvid's address space
// at 'dstva' in dstenvid's address space with permission 'perm'.
// Perm has the same restrictions as in sys_page_alloc, except
// that it also must not grant write access to a read-only
// page.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if srcenvid and/or dstenvid doesn't currently exist,
//		or the caller doesn't have permission to change one of them.
//	-E_INVAL if srcva >= UTOP or srcva is not page-aligned,
//		or dstva >= UTOP or dstva is not page-aligned.
//	-E_INVAL is srcva is not mapped in srcenvid's address space.
//	-E_INVAL if perm is inappropriate (see sys_page_alloc).
//	-E_INVAL if (perm & PTE_W), but srcva is read-only in srcenvid's
//		address space.
//	-E_NO_MEM if there's no memory to allocate any necessary page tables.
static int
sys_page_map(envid_t srcenvid, void *srcva,
		 envid_t dstenvid, void *dstva, int perm)
{
	// Hint: This function is a wrapper around page_lookup() and
	//   page_insert() from kern/pmap.c.
	//   Again, most of the new code you write should be to check the
	//   parameters for correctness.
	//   Use the third argument to page_lookup() to
	//   check the current permissions on the page.

	// LAB 4: Your code here.
	// panic("sys_page_map not implemented");

	struct Env *senv, *denv;
	int r;

	if ((r = envid2env(srcenvid, &senv, 1)) < 0 || (r = envid2env(dstenvid, &denv, 1)) < 0)
		return -E_BAD_ENV;

	if ((uint32_t) srcva >= UTOP || (uint32_t) dstva >= UTOP || (uint32_t) srcva % PGSIZE != 0 || (uint32_t) dstva % PGSIZE != 0)
		return -E_INVAL;


	if ((perm | PTE_U | PTE_P) != perm || (perm & ~PTE_SYSCALL) != 0)
		return -E_INVAL;
	
	pte_t * pte;
	struct PageInfo *pp;

	// look up for the page corresponding to srcva
	if ((pp = page_lookup(senv->env_pgdir, srcva, &pte)) == NULL)
		return -E_INVAL;

	// if page table entry is not writable but perm contains PTE_W
	if (((perm & PTE_W) == PTE_W) && ((*pte & PTE_W) == 0))
		return -E_INVAL;

	if ((r = page_insert(denv->env_pgdir, pp, dstva, perm)) < 0)
		return -E_NO_MEM;

	return 0;
}

// Unmap the page of memory at 'va' in the address space of 'envid'.
// If no page is mapped, the function silently succeeds.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if va >= UTOP, or va is not page-aligned.
static int
sys_page_unmap(envid_t envid, void *va)
{
	// Hint: This function is a wrapper around page_remove().

	// LAB 4: Your code here.
	// panic("sys_page_unmap not implemented");

	struct Env *e;
	int r;

	if ((uint32_t) va >= UTOP || (uint32_t) va % PGSIZE != 0)
		return -E_INVAL;

	if ((r = envid2env(envid, &e, 1)) < 0)
		return -E_BAD_ENV;

	page_remove(e->env_pgdir, va);
	return 0;
}

// Try to send 'value' to the target env 'envid'.
// If srcva < UTOP, then also send page currently mapped at 'srcva',
// so that receiver gets a duplicate mapping of the same page.
//
// The send fails with a return value of -E_IPC_NOT_RECV if the
// target is not blocked, waiting for an IPC.
//
// The send also can fail for the other reasons listed below.
//
// Otherwise, the send succeeds, and the target's ipc fields are
// updated as follows:
//    env_ipc_recving is set to 0 to block future sends;
//    env_ipc_from is set to the sending envid;
//    env_ipc_value is set to the 'value' parameter;
//    env_ipc_perm is set to 'perm' if a page was transferred, 0 otherwise.
// The target environment is marked runnable again, returning 0
// from the paused sys_ipc_recv system call.  (Hint: does the
// sys_ipc_recv function ever actually return?)
//
// If the sender wants to send a page but the receiver isn't asking for one,
// then no page mapping is transferred, but no error occurs.
// The ipc only happens when no errors occur.
//
// Returns 0 on success, < 0 on error.
// Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist.
//		(No need to check permissions.)
//	-E_IPC_NOT_RECV if envid is not currently blocked in sys_ipc_recv,
//		or another environment managed to send first.
//	-E_INVAL if srcva < UTOP but srcva is not page-aligned.
//	-E_INVAL if srcva < UTOP and perm is inappropriate
//		(see sys_page_alloc).
//	-E_INVAL if srcva < UTOP but srcva is not mapped in the caller's
//		address space.
//	-E_INVAL if (perm & PTE_W), but srcva is read-only in the
//		current environment's address space.
//	-E_NO_MEM if there's not enough memory to map srcva in envid's
//		address space.
static int
sys_ipc_try_send(envid_t envid, uint32_t value, void *srcva, unsigned perm)
{
	// LAB 4: Your code here.
	int r;
	struct Env * dst_env;

	if ((r = envid2env(envid,&dst_env,0)) < 0)
		return -E_BAD_ENV;

	if (dst_env->env_ipc_recving == 0)
		return -E_IPC_NOT_RECV;

	if ((uintptr_t) srcva < UTOP){
		if ((uintptr_t) srcva % PGSIZE != 0)
			return -E_INVAL;

		if ((perm & (PTE_U | PTE_P)) != (PTE_U | PTE_P) || (perm | PTE_SYSCALL) != PTE_SYSCALL)
			return -E_INVAL;

		struct PageInfo * pp;
		pte_t * pte;
		if ((pp = page_lookup(curenv->env_pgdir, srcva, &pte)) == NULL)
			return -E_INVAL;
	
		if ((perm & PTE_W) && (*pte & PTE_W) == 0)
			return -E_INVAL;

		// send mapping if all good
		if (dst_env->env_ipc_dstva) {
			if ((r = page_insert(dst_env->env_pgdir, pp, dst_env->env_ipc_dstva, perm)) < 0)
				return -E_NO_MEM;
			dst_env->env_ipc_perm = perm;
		}
	} else {
		dst_env->env_ipc_perm = 0;
	}
	
	dst_env->env_ipc_recving = 0;
	dst_env->env_ipc_value = value;
	dst_env->env_ipc_from = curenv->env_id;
	dst_env->env_status = ENV_RUNNABLE;

	return 0;
}

// Block until a value is ready.  Record that you want to receive
// using the env_ipc_recving and env_ipc_dstva fields of struct Env,
// mark yourself not runnable, and then give up the CPU.
//
// If 'dstva' is < UTOP, then you are willing to receive a page of data.
// 'dstva' is the virtual address at which the sent page should be mapped.
//
// This function only returns on error, but the system call will eventually
// return 0 on success.
// Return < 0 on error.  Errors are:
//	-E_INVAL if dstva < UTOP but dstva is not page-aligned.
static int
sys_ipc_recv(void *dstva)
{
	// LAB 4: Your code here.
	if ((uintptr_t) dstva < UTOP && (uintptr_t) dstva % PGSIZE != 0)
		return -E_INVAL;

	curenv->env_ipc_recving = 1;
	curenv->env_ipc_dstva = dstva;
	curenv->env_status = ENV_NOT_RUNNABLE;
	
	return 0;
}

// system call to obtain the date. Most of the changes done to implement this are done in
// lapic.c and date.h. Also, to impelent the lapic.c we implemented most part by taking inspiration
// from lapic.c of xv6 code. 
static int
sys_date(struct rtcdate* rd)
{
	cmostime(rd);
	return 0;
}

// had to make this new function after much debugging because there was error in envid2env 
// with 1 as argument.
// with 1 as argument only parent can destroy child, but with 0 any process can kill
static int
sys_kill(envid_t envid)
{
	int r;
	struct Env *e;

	if ((r = envid2env(envid, &e, 0)) < 0)
		return r;
	if (e == curenv)
		cprintf("[%08x] exiting gracefully\n", curenv->env_id);
	else
		cprintf("[%08x] destroying %08x\n", curenv->env_id, e->env_id);
	env_destroy(e);
	return 0;
}

static int
sys_trace(envid_t child_envid)
{
	int r;
	struct Env *childEnv;
	if ((r = envid2env(child_envid, &childEnv, 0)) != 0){
		panic("Error im sys_trace() : %e",r);
	}

	childEnv->env_status = ENV_RUNNABLE;
	curenv->env_status = ENV_NOT_RUNNABLE;
	env_run(childEnv);
	sched_yield();	
}

static void
sys_trace_me()
{
	curenv->env_being_traced = 1;
	int r;
	struct Env *parentEnv;

	if ((r = envid2env(curenv->env_parent_id, &parentEnv, 0)) != 0){
		panic("Error in sys_trace_me() : %e",r);
	}

	parentEnv->env_status = ENV_RUNNABLE;
	curenv->env_status = ENV_NOT_RUNNABLE;
	sched_yield();
}

static void 
sys_oprof_me()
{
	curenv->env_being_oprofed = 1;
	int r;
	struct Env *parentEnv;

	if ((r = envid2env(curenv->env_parent_id, &parentEnv, 0)) != 0){
		panic("Error in sys_oprof_me() : %e",r);
	}

	parentEnv->env_status = ENV_RUNNABLE;
	curenv->env_status = ENV_NOT_RUNNABLE;
	sched_yield();
}

static void
sys_get_child_tf(envid_t child_envid,struct Trapframe * tf)
{
	int r;
	struct Env *childEnv;

	if ((r = envid2env(child_envid, &childEnv, 0)) != 0){
		panic("Error in sys_get_child_tf() : %e",r);

	}
	memcpy((void*)tf, (void*)(&(childEnv->env_tf)),sizeof(struct Trapframe)) ;
	return;
}

static void
sys_ptrace_attach()
{
	curenv->env_ptrace_attached = 1;
}

static int
sys_ptrace_peek(envid_t child_envid, void * instr_addr, void * data){

	int r;
	struct Env *childEnv;

	if ((r = envid2env(child_envid, &childEnv, 0)) != 0){
		panic("Error in sys_ptrace_peek() : %e",r);
	}

	lcr3(PADDR(childEnv->env_pgdir));
	int temp = *(int*)instr_addr;
	lcr3(PADDR(curenv->env_pgdir));
	*(int*)data = temp;

	return 0;
}

static int
sys_ptrace_poke(envid_t child_envid, void * instr_addr, void * data){

	int r;
	struct Env *childEnv;

	if ((r = envid2env(child_envid, &childEnv, 0)) != 0){
		panic("Error in sys_ptrace_peek() : %e",r);
	}

	int temp = *(int*)data;
	lcr3(PADDR(childEnv->env_pgdir));
	*(int*)instr_addr = temp;
	lcr3(PADDR(curenv->env_pgdir));

	return 0;
}

static int
sys_ptrace_set_debug_flag(envid_t child_envid,int flag){

	int r;
	struct Env *childEnv;

	if ((r = envid2env(child_envid, &childEnv, 0)) != 0){
		panic("Error in sys_ptrace_peek() : %e",r);
	}

	if(flag==1)
		childEnv->env_tf.tf_eflags |= FL_TF;
	else if (flag==0)
		childEnv->env_tf.tf_eflags &= (~(FL_TF));
	else
		panic("sys_ptrace_set_debug_flag only take flag as either 0 or 1");
	
	return 0;
}

static int
sys_ptrace_set_eip(envid_t child_envid,void* instr_ptr){
	int r;
	struct Env *childEnv;

	if ((r = envid2env(child_envid, &childEnv, 0)) != 0){
		panic("Error in sys_ptrace_peek() : %e",r);
	}
	childEnv->env_tf.tf_eip = (uintptr_t)instr_ptr;
	return 0;
}

//function sys_exec for the shell
static int sys_exec(char *func,char **argv)
{
	#define MAXARG 11
	#define ENV_PASTE3(x, y, z) x ## y ## z
	uint8_t *binary;

	//define binaries for different cases
	if(strcmp(func,"fact") == 0) {
		extern uint8_t ENV_PASTE3(_binary_obj_, user_fact, _start)[];
		binary=ENV_PASTE3(_binary_obj_, user_fact, _start);	
	} else if(strcmp(func,"fib") == 0) {
		extern uint8_t ENV_PASTE3(_binary_obj_, user_fib, _start)[];
		binary=ENV_PASTE3(_binary_obj_, user_fib, _start);	
	} else if(strcmp(func,"echo") == 0) {
		extern uint8_t ENV_PASTE3(_binary_obj_, user_echo, _start)[];
		binary=ENV_PASTE3(_binary_obj_, user_echo, _start);	
	} else if (strcmp(func,"date") == 0) {
		extern uint8_t ENV_PASTE3(_binary_obj_, user_date, _start)[];
		binary=ENV_PASTE3(_binary_obj_, user_date, _start);
	} else if (strcmp(func,"kill") == 0) {
		extern uint8_t ENV_PASTE3(_binary_obj_, user_kill, _start)[];
		binary=ENV_PASTE3(_binary_obj_, user_kill, _start);
	} else if (strcmp(func,"help") == 0) {
		extern uint8_t ENV_PASTE3(_binary_obj_, user_help, _start)[];
		binary=ENV_PASTE3(_binary_obj_, user_help, _start);
	} else if (strcmp(func,"strace") == 0) {
		extern uint8_t ENV_PASTE3(_binary_obj_, user_strace, _start)[];
		binary=ENV_PASTE3(_binary_obj_, user_strace, _start);
	} else if (strcmp(func,"attach") == 0) {
		extern uint8_t ENV_PASTE3(_binary_obj_, user_ptrace, _start)[];
		binary=ENV_PASTE3(_binary_obj_, user_ptrace, _start);
	} else if (strcmp(func,"oprof") == 0) {
		extern uint8_t ENV_PASTE3(_binary_obj_, user_oprof, _start)[];
		binary=ENV_PASTE3(_binary_obj_, user_oprof, _start);
	}else {
		cprintf("\nCommand %s does not exist! Refer to following help section...\n\n",func);
		extern uint8_t ENV_PASTE3(_binary_obj_, user_help, _start)[];
		binary=ENV_PASTE3(_binary_obj_, user_help, _start);
	}
	/*else if (strcmp(argv[1],"hello")==0 && strcmp(func,"attach")==0) {
		// cprintf("came into this \n");
		extern uint8_t ENV_PASTE3(_binary_obj_, user_hello, _start)[];
		binary=ENV_PASTE3(_binary_obj_, user_hello, _start);
	}
	*/

	//in load icode we loaded the kernel's page directory, so put back the user page directory
	//need to keep a seperate temporay stack to keep the pointers to the arguments pushed onto
	//the stack
	uint32_t argc;
	uint32_t sp = USTACKTOP;
	uint32_t ustack[2+MAXARG+1];

	//iterate till there are some arguments on the stack
	for(argc = 0; argv[argc]; argc++) {
		if(argc >= MAXARG) {
		  cprintf("argc and len of argv don't match\n");
		  return -1;
		}
		sp = (sp - (strlen(argv[argc]) + 1)) & ~3;
		memcpy((void *)sp, (void *)argv[argc], strlen(argv[argc]) + 1);
		ustack[2+argc] = sp;
	}

	//make the cell on the top of the pushed arguments as null
	ustack[2+argc] = 0;

	//push the argc and pointer to argv
	ustack[0] = argc;
	ustack[1] = sp - (argc+1)*4;  // argv pointer
	
	//now simply copy the temporary stack to the stack of the process
	sp -= (2+argc+1)*4;
	memcpy((void *)sp, (void *)ustack, (2+argc+1)*4);
	curenv->env_tf.tf_esp = sp;
	
	//load the binary to the process
	my_load_icode(curenv,binary);
	// lcr3(PADDR(curenv->env_pgdir));

	// for ptrace
	if (curenv->env_ptrace_attached != 1){
		env_run(curenv);
	} else{
		int r;
		struct Env *parentEnv;
		if ((r = envid2env(curenv->env_parent_id, &parentEnv, 0)) != 0){
			panic("Error in sys_exec() : %e",r);
		}
		curenv->env_status = ENV_NOT_RUNNABLE;
		parentEnv->env_status = ENV_RUNNABLE;
		sched_yield();
	}

	return 0;
}


// Dispatches to the correct kernel function, passing the arguments.
int32_t
syscall(uint32_t syscallno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
	// Call the function corresponding to the 'syscallno' parameter.
	// Return any appropriate return value.
	// LAB 3: Your code here.

	// panic("syscall not implemented");

	switch (syscallno) {
		case (SYS_cputs):
			sys_cputs((const char *)a1, (size_t) a2);
			break;
		case(SYS_cgetc):
			return sys_cgetc();
		case (SYS_getenvid):
			return sys_getenvid();
		case(SYS_env_destroy):
			return sys_env_destroy(a1);
		case (SYS_yield):
			sys_yield();
			break;
		case (SYS_exofork):
			return sys_exofork();
		case (SYS_env_set_status):
			return sys_env_set_status(a1, a2);
		case (SYS_page_alloc):
			return sys_page_alloc(a1, (void *) a2, a3);
		case (SYS_page_map):
			return sys_page_map(a1, (void *) a2, a3, (void *) a4, a5);
		case (SYS_page_unmap):
			return sys_page_unmap(a1, (void *) a2);
		case (SYS_env_set_pgfault_upcall):
			return sys_env_set_pgfault_upcall(a1, (void *) a2);
		case (SYS_ipc_try_send):
			return sys_ipc_try_send(a1, a2, (void *) a3, a4);
		case (SYS_ipc_recv):
			return sys_ipc_recv((void *) a1);
		case (SYS_exec):
			return sys_exec((char *)a1,(char **)a2);
		case (SYS_date):
			return sys_date((struct rtcdate*)a1);
		case(SYS_wait):
			sys_wait();
			return 0;
		case(SYS_kill):
			return sys_kill((envid_t)a1);
		case(SYS_trace_me):
			sys_trace_me();
			return 0;
		case(SYS_oprof_me):
			sys_oprof_me();
			return 0;
		case(SYS_trace):
			return sys_trace((envid_t)a1);
		case(SYS_get_child_tf):
			sys_get_child_tf((envid_t)a1,(struct Trapframe *)a2);
			return 0;
		case(SYS_ptrace_attach):
			sys_ptrace_attach();
			return 0;
		case(SYS_ptrace_peek):
			return sys_ptrace_peek((envid_t)a1,(void*)a2,(void*)a3);
		case(SYS_ptrace_poke):
			return sys_ptrace_poke((envid_t)a1,(void*)a2,(void*)a3);
		case(SYS_ptrace_set_debug_flag):
			return sys_ptrace_set_debug_flag((envid_t)a1,(int)a2);
		case (SYS_ptrace_set_eip):
			return sys_ptrace_set_eip((envid_t)a1,(void*)a2);
		default:
			return -E_INVAL;
	}
	return 0;
}

