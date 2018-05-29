#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/tlb.h>


/* Place your page table functions here */


void vm_bootstrap(void)
{
        /* Initialise VM sub-system.  You probably want to initialise your 
           frame table here as well.
        */

		
		
		//Add*******************************************************
		init_frametable();
		//*******************************************************

}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
        (void) faulttype;
        (void) faultaddress;

        //panic("vm_fault hasn't been written yet\n");
		switch(faulttype){
			case VM_FAULT_READ:
				break;
			case VM_FAULT_WRITE:
				break;
			case VM_FAULT_READONLY:
				return EFAULT;
			default:
				return EINVAL;
		}



        return EFAULT;
}

/*
 *
 * SMP-specific functions.  Unused in our configuration.
 */

void
vm_tlbshootdown(const struct tlbshootdown *ts)
{
        (void)ts;
        panic("vm tried to do tlb shootdown?!\n");
}

