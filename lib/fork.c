// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	if ((uvpt[PGNUM(addr)] & PTE_COW) != PTE_COW || (err & FEC_WR) != FEC_WR) 
		panic("Error in pgfault : either error not FEC_WR or page not PTE_COW");

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.

	// alloc temp page
	if ((r = sys_page_alloc(0, PFTEMP, PTE_U | PTE_W | PTE_P)) < 0)
		panic("Error in pgfault : %e", r);
	
	// memmove page to temp page
	void * rounded = ROUNDDOWN(addr,PGSIZE);
	memmove(PFTEMP, rounded, PGSIZE);
	
	if ((r = sys_page_map(0, PFTEMP, 0, rounded, PTE_U | PTE_P | PTE_W)) < 0)
		panic("Error in pgfault : %e", r);
	
	if ((r = sys_page_unmap(0, PFTEMP)) < 0)
		panic("Error in pgfault : %e", r);

	return;
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;

	// LAB 4: Your code here.

	void * va = (void *) (pn * PGSIZE);

	// PTE_W | PTE_COW
	if ((uvpt[pn] & PTE_W) == PTE_W || (uvpt[pn] & PTE_COW) == PTE_COW){

		if ((r = sys_page_map(0, va, envid, va, PTE_U | PTE_COW | PTE_P)) < 0)
			panic("Error in duppage : sys_page_map : %e",r);
	 
		if ((r = sys_page_map(0, va, 0, va, PTE_U | PTE_COW | PTE_P)) < 0) 
			panic("Error in duppage : sys_page_map : %e",r);
	} else {
		if ((r = sys_page_map(0, va, envid, va, uvpt[pn] & 0xfff)) < 0)
			panic("Error in duppage : sys_page_map : %e",r);
	}

	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.

	set_pgfault_handler(pgfault);

	envid_t new_env_id;
	if ((new_env_id = sys_exofork()) < 0)
		panic("Error in fork in sys_exofork() : %e",new_env_id);

	// parent process
	if (new_env_id > 0){
		uintptr_t addr;
		for (addr = UTEXT; addr < USTACKTOP; addr += PGSIZE){
			if ((uvpd[PDX(addr)] & PTE_P) && (uvpt[PGNUM(addr)] & PTE_P)) {
				duppage(new_env_id, PGNUM(addr));
			}
		}

		int r;
		// exception stack
		if ((r = sys_page_alloc(new_env_id, (void*)(UXSTACKTOP-PGSIZE), PTE_U | PTE_P | PTE_W)) < 0)
			panic("Error in fork : sys_page_alloc error : %e", r);

		// set child page fault handler
		if ((r = sys_env_set_pgfault_upcall(new_env_id, (void*)thisenv->env_pgfault_upcall)) < 0)
			panic("Error in fork : sys_env_set_pgfault_upcall error : %e", r);

		// set child runnable
		if ((r = sys_env_set_status(new_env_id, ENV_RUNNABLE)) < 0)
			panic("Error in fork : sys_env_set_status error : %e", r);

		return new_env_id;
	}

	// else we are in child process
	thisenv = &envs[ENVX(sys_getenvid())];
	return 0;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
