#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/tlb.h>
//ADD
#include <spl.h>
#include <proc.h>
//

/* Place your page table functions here */

void init_hpt(void)
{
	paddr_t top_of_ram = ram_getsize();
	
	// Get the number of frames		
	int num_of_frames = (top_of_ram)/PAGE_SIZE;
	
	// Allocate two times of slots for each frame
	hpt_size = 2*num_of_frames;
	//kprintf("hpt_size is %d\n",hpt_size);
	// Bump allocator will be used
	hash_page_table = (struct hpt_entry*) kmalloc(sizeof(struct hpt_entry)*hpt_size);
	KASSERT(hash_page_table!=NULL);

	for(int i=0; i<hpt_size; ++i)
	{
		hash_page_table[i].pid = -1;
		hash_page_table[i].VPN = 0;
		hash_page_table[i].PFN = 0;
		hash_page_table[i].next = NULL;
	}
}

uint32_t hpt_hash(struct addrspace *as, vaddr_t faultaddr)
{
        uint32_t index;

        index = (((uint32_t )as) ^ (faultaddr >> PAGE_BITS)) % hpt_size;
        return index;
}


void vm_bootstrap(void)
{
        /* Initialise VM sub-system.  You probably want to initialise your 
           frame table here as well.
        */
		
		//Add*******************************************************
		// init hpt first, so will use bump allocator
		init_hpt();
		init_frametable();
		//*******************************************************

}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
        (void) faulttype;
        (void) faultaddress;
		
		int f;
		//int res;
		uint32_t dir_bit = 0; //Mark
		uint32_t low_entry;
		uint32_t high_entry;


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

		struct addrspace *curr_addr = proc_getas();
		paddr_t** pt_pointer = curr_addr->pageTable;


		if(curr_addr == NULL || pt_pointer == NULL){
			return EFAULT;
		}



		paddr_t fault_paddr = KVADDR_TO_PADDR(faultaddress);

		uint32_t root_index = fault_paddr >> 22;		
		uint32_t second_index = fault_paddr << 10 >> 22;

		if(pt_pointer[root_index] == NULL){
			//res = vm_add_root_ptentry(pt_pointer, root_index);
			
			//Initialize the entry for root page table;
			pt_pointer[root_index] = kmalloc(sizeof(paddr_t)*SIZE_OF_PAGETABLE);
			if(pt_pointer[root_index] == NULL){
				return ENOMEM;
			}			
			for(int i = 0; i < SIZE_OF_PAGETABLE; i++){
				pt_pointer[root_index][i] = 0;			
			}

			/*
			if(res){
				return res;
			}*/
			f = true;
		}

		if(pt_pointer[root_index][second_index] == 0){
			struct region* curr = curr_addr->regionList;
			while(curr != NULL){
				if((curr->vir_base + (curr->num_of_pages * PAGE_SIZE) > faultaddress) && (curr->vir_base <= faultaddress)){
					if(curr->w_bit == 0){
						dir_bit = 0;
					}else{
						dir_bit = TLBLO_DIRTY;
					}
					break;
				}
				curr = curr->next;
			}
			if(curr == NULL){
				if(f == true){
					kfree(pt_pointer[root_index]);
				}
				return EFAULT;
			}
			
			//result = vm_add_ptentery();

			vaddr_t temp_alloc = alloc_kpages(1);
			if(temp_alloc == 0){
				if(f == true){
					kfree(pt_pointer[root_index]);
				}
				return ENOMEM;
			}else{
				paddr_t temp_phy_alloc = KVADDR_TO_PADDR(temp_alloc);
				pt_pointer[root_index][second_index] = (temp_phy_alloc & PAGE_FRAME) | dir_bit | TLBLO_VALID;
			}
			
			

			/*
			if(res){
				if(flag == true){
					kfree(pagetable[root_index]);				
				}
				return result;
			}*/
			
		}

		low_entry = pt_pointer[root_index][second_index];
		high_entry = faultaddress & PAGE_FRAME;	

		int s = splhigh();
		tlb_random(high_entry, low_entry);
		splx(s);
		return 0;
        
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

