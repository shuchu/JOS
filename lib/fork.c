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

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.

	//panic("pgfault not implemented");
        
        if ((err & FEC_WR) == 0 ||
            (uvpd[PDX(addr)] & PTE_P) == 0 ||
            (uvpt[PTX(addr)] & PTE_COW) == 0 )
            panic("Error. pgfault() not a write or attempting to access a non-cow page.");

        if ((r = sys_page_alloc(0, (void*) PFTEMP, PTE_U|PTE_P|PTE_W)) < 0)
            panic("Error. pgfault() page_allocation on temp memeory failed %e", r);

        addr = ROUNDDOWN(addr, PGSIZE);

        memmove(PFTEMP, addr, PGSIZE);

        if ((r = sys_page_map(0, PFTEMP, 0, addr, PTE_U|PTE_P|PTE_W)) < 0)
            panic("Error, pgfault() page mapping failed %e", r);

        if ((r = sys_page_unmap(0, PFTEMP)) < 0)
            panic("Error, pgfault() unmap temp memory failed. %e", r);
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
	//panic("duppage not implemented");
        
        void *addr = (void*) ((uintptr_t) pn * PGSIZE);
        pte_t pte = uvpt[PTX(addr)];

        if ((pte & PTE_W) > 0 || (pte & PTE_COW) > 0) {
            if ((r = sys_page_map(0, addr, envid, addr, PTE_U|PTE_P|PTE_COW)) < 0 )
                panic("duppage, page re-mapping failed, %e ", r);

            if ((r = sys_page_map(0, addr, 0, addr, PTE_U|PTE_P|PTE_COW)) < 0 )
                panic("duppage, page re-mapping failed, %e ", r);
        } else {
            if ((r = sys_page_map(0, addr, envid, addr, PTE_U|PTE_P)) < 0 )
                panic("duppage, page re-mapping failed, %e ", r);
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
	//panic("fork not implemented");

        // set pgfault handler
        set_pgfault_handler(pgfault);

        // create a child
        envid_t child_env_id = sys_exofork();
        
        if (child_env_id < 0)
            panic("Failed to create a child process %e", child_env_id);

        if (child_env_id == 0) {
            thisenv = &envs[ENVX(sys_getenvid())];
            return 0;
        }

        // we are the parent
        uintptr_t addr;
        for (addr = UTEXT; addr < UTOP - PGSIZE; addr += PGSIZE) {
            if ((uvpd[PDX(addr)] & PTE_P) > 0 &&
                (uvpt[PTX(addr)] & PTE_P) > 0 &&
                (uvpt[PTX(addr)] & PTE_U) > 0)
                duppage(child_env_id, PTX(addr));
        }
        
        int r;
        if ((r = sys_page_alloc(child_env_id, (void*)(UTOP-PGSIZE), PTE_U|PTE_W|PTE_P)) < 0)
            panic("fork, page_allocation for exception stak fail, %e", r);


        extern void _pgfault_upcall(void);

        sys_env_set_pgfault_upcall(child_env_id, _pgfault_upcall);

        // start the child env
        if ((r = sys_env_set_status(child_env_id, ENV_RUNNABLE)) < 0)
            panic("for, set child env to be runnable fail, %e", r);

        return child_env_id;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
