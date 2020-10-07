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
	if (!(err & FEC_WR) || !(uvpt[(uintptr_t)addr/PGSIZE] & PTE_COW)) {
		// if (!(err & FEC_WR))
		// 	cprintf("not a write\n");
		// else 
		// 	cprintf(" not to a COW page\n");
		panic("faulting access was not a write, or not to a COW page\n");
	}

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
	// 这里要使用sys_getenvid，调试过程中发现：在父进程被free之后、子进程int后的第一条指令被执行之前，
	// 处理器会运行到这里的pgfault（暂不清楚为什么会这样？？），此时处理器执行的是子进程，但thisenv还没有来得及更新！！
	envid_t envid = sys_getenvid(); //WHY !!!!!!!! 什么时候能用thisenv->env_id什么时候不能？？
	// cprintf("thisenv->envid: %d\n", thisenv->env_id);
	// cprintf("sys_getenvid(): %d\n", sys_getenvid());
	if ((r = sys_page_alloc(envid, PFTEMP, PTE_P|PTE_U|PTE_W)) < 0)
		panic("sys_page_alloc: %e", r);
	// 这里是页错误处理函数，遇到的addr不一定是页对齐的，需要手动ROUNDDOWN
	memmove(PFTEMP,ROUNDDOWN(addr, PGSIZE), PGSIZE);
	if ((r = sys_page_map(envid, PFTEMP, envid, ROUNDDOWN(addr, PGSIZE), PTE_P|PTE_U|PTE_W)) < 0)
		panic("sys_page_map: %e", r);
	if ((r = sys_page_unmap(envid, PFTEMP)) < 0)
		panic("sys_page_unmap: %e", r);

	// panic("pgfault not implemented");
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
	// BUT: Why do we need to mark ours copy-on-write again if it was already
	// 		copy-on-write at the beginning of this function?
	uintptr_t *addr = (uintptr_t *)(pn * PGSIZE);
	int basic_perm = PTE_U | PTE_P;

	// if ((uintptr_t)addr % 0x100000 == 0)	
	// 		cprintf("addr: %x, so far so good\n", addr);

	if ((uvpt[pn] & PTE_W) || (uvpt[pn] & PTE_COW)) {
		if ((r = sys_page_map(thisenv->env_id, addr, envid, addr, basic_perm|PTE_COW)) < 0)
			panic("sys_page_map: %e", r);
		if ((r = sys_page_map(thisenv->env_id, addr, thisenv->env_id, addr, basic_perm|PTE_COW)) < 0)
			panic("sys_page_map: %e", r);
	} else {
		if ((r = sys_page_map(thisenv->env_id, addr, envid, addr, basic_perm)) < 0)
			panic("sys_page_map: %e", r);
	}
	// panic("duppage not implemented");
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
	int r;
	envid_t envid;
	uintptr_t addr;
	void _pgfault_upcall();

	set_pgfault_handler(pgfault);
	envid = sys_exofork();
	if (envid < 0) 
		panic("sys_exofork: %e", envid);
	if (envid == 0) {
		// we're the child
		thisenv = &envs[ENVX(sys_getenvid())]; //fix "thisenv" in the child process.
		return 0;
	}
	// We're the parent
	for (addr = UTEXT; addr < USTACKTOP; addr += PGSIZE) {
		if ((uvpd[addr/PTSIZE] & (PTE_U|PTE_P)) == (PTE_U|PTE_P) && 
				(uvpt[addr/PGSIZE] & (PTE_U|PTE_P)) == (PTE_U|PTE_P))
			duppage(envid, addr/PGSIZE);
	}
	// 一定不能在子进程中安装pgfault处理函数，因为在那之前父进程还没有创建子进程的地址空间映射！！！！
	// 由于父进程负责设置子进程的RUNNABLE状态，所以父进程执行完fork后，子进程的fork才会被调度
	if((r = sys_page_alloc(envid, (void *)(UXSTACKTOP-PGSIZE), PTE_P|PTE_U|PTE_W)) < 0)
		panic("sys_page_alloc: %e", r);
	if ((r = sys_env_set_pgfault_upcall(envid, _pgfault_upcall) < 0))
		panic("sys_env_set_pgfault_upcall failed\n");
	if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
		panic("sys_env_set_status: %e", envid);

	return envid;
	// panic("fork not implemented");
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
