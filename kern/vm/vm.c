#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/tlb.h>

#include <frametable.h> //Add

/* Place your page table functions here */


void vm_bootstrap(void)
{
        /* Initialise VM sub-system.  You probably want to initialise your 
           frame table here as well.
        */

		
		
		//Add*******************************************************
		//paddr_t top_of_ram = ran_getsize();

		//Get the number of frames
		unsigned int size_of_frames = ran_getsize()/PAGE_SIZE;
		paddr_t bound = ran_getsize() - (size_of_frames * sizeof(struct frameTable));

		//Frame table		
		ft_table = (struct ft_entry* ) PADDR_TO_KVADDR();

		//Create frame table
		struct ft_entry ft_e;
		for(int i = 0; i < size_of_frames; i++){
			
			if(i == 0){
				ft_e.prev = size_of_frames - 1;				
			}else{
				
				ft_e.prev = i-1;
			}
			
			if(i == size_of_frames-1){
				ft_e.next = 0;
			}else{
				ft_e.next = i+1;
			}

			fte_entry.used = false;
		
			memmove(&ft_table[i], &fte_entry, sizeof(ft_entry));
		}
		//*******************************************************

}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
        (void) faulttype;
        (void) faultaddress;

        panic("vm_fault hasn't been written yet\n");

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

