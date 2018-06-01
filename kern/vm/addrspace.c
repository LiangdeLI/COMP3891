/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *        The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <spinlock.h>
#include <current.h>
#include <mips/tlb.h>
#include <addrspace.h>
#include <vm.h>
#include <proc.h>

/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 *
 * UNSW: If you use ASST3 config as required, then this file forms
 * part of the VM subsystem.
 *
 */

struct addrspace *
as_create(void)
{
    struct addrspace *as;

    as = kmalloc(sizeof(struct addrspace));
    if (as == NULL) {
            return NULL;
    }

    /*
     * Initialize as needed.
     */

	as->regionList = NULL;		

    return as;
}

int
as_copy(struct addrspace *old, struct addrspace **ret)
{
        struct addrspace *new_addr;

        new_addr = as_create();
        if (new_addr==NULL) {
                return ENOMEM;
        }

        //Add*****************
        struct region* old_region = old->regionList;
		struct region* new_region = new_addr->regionList;

		while(old_region != NULL){
			struct region* reg = kmalloc(sizeof(struct region));
			if(reg == NULL){
				//as_destroy();
				return ENOMEM;			
			}else{
				reg->next = NULL;
				reg->vir_base = old_region -> vir_base;				
				reg->writeable = old_region->writeable;
				reg->prev_writeable = old_region->prev_writeable;
				reg->num_of_pages = old_region->num_of_pages;

				if(new_region == NULL){
					new_addr->regionList = reg;
				}else{
					new_region->next = reg;
				} 
				new_region = reg;
				old_region = old_region->next;
			}

			
		}

		for(int i = 0; i < SIZE_OF_PAGETABLE;i++){
			if(old->pageTable == NULL){
				continue;
			}
			new_addr->pageTable[i] = kmalloc(sizeof(paddr_t) * SIZE_OF_PAGETABLE);
			for(int j = 0; j < SIZE_OF_PAGETABLE; j++){

				if(old->pageTable[i][j] == 0){
					new_addr->pageTable[i][j] = 0;				
				}else{
					vaddr_t frame_addr_new = alloc_kpages(1);
					//vaddr_t frame_addr_old = PADDR_TO_KVADDR(old->pageTable[i][j] & PAGE_FRAME)
					memmove((void*)frame_addr_new, (const void *)PADDR_TO_KVADDR(old->pageTable[i][j] & PAGE_FRAME), PAGE_SIZE);
					//dirty = 
					new_addr->pageTable[i][j] = (PAGE_FRAME & KVADDR_TO_PADDR(frame_addr_new)) | TLBLO_VALID | (old->pageTable[i][j] & TLBLO_DIRTY);
				}
				
			}
		}


        //*******************

        //(void)old;

        *ret = new_addr;
        return 0;
}

void
as_destroy(struct addrspace *as)
{
	/*
    * Clean up as needed.
    */

	//Free all regions with their hpt_entrys and physical memory
	struct region* prev;
	struct region* cur;
	cur = as->regionList;
	prev = cur;
		
	while(cur != NULL){
		cur = cur->next;
		region_destroy(as, prev);
		KASSERT(prev==NULL);
		prev = cur;
	}

	if(prev!=NULL){
		region_destroy(as, prev);
		KASSERT(prev==NULL);
	}

    kfree(as);
    KASSERT(as==NULL);
}

void
as_activate(void)
{
    struct addrspace *as;

    as = proc_getas();
    if (as == NULL) {
            /*
             * Kernel thread without an address space; leave the
             * prior address space in place.
             */
            return;
    }

    /*
     * Write this.
     */
	//Add*********************************
	int s = splhigh();
	
	//flush the TLB		
	for(int i = 0; i < NUM_TLB; i++){
		tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);		
	}

	splx(s);
	//************************************

}

void
as_deactivate(void)
{
    /*
     * Write this. For many designs it won't need to actually do
     * anything. See proc.c for an explanation of why it (might)
     * be needed.
     */

	//Add*********************************
	as_activate();
	//************************************
}

/*
 * Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
 *
 * The READABLE, WRITEABLE, and EXECUTABLE flags are set if read,
 * write, or execute permission should be set on the segment. At the
 * moment, these are ignored. When you write the VM system, you may
 * want to implement them.
 */
int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t memsize,
                 int readable, int writeable, int executable)
{
    /*
     * Write this.
     */

	//Add************************************
	memsize += vaddr & ~(vaddr_t)PAGE_FRAME;
	vaddr &= PAGE_FRAME;

	memsize = (memsize + PAGE_SIZE -1) & PAGE_FRAME;

	size_t num_of_pages = memsize / PAGE_SIZE;

	struct region * new_region 
			= region_create(vaddr, num_of_pages, readable, writeable, executable);
	if (new_region == NULL) {
    	return ENOMEM;
	}

	as_add_region(as, new_region);

	return 0;
	//***************************************
}

void as_add_region(struct addrspace *as, struct region *new_region)
{
	struct region* prev;
	struct region* curr; 

	if(as->regionList != NULL){
		curr = as->regionList;
		prev = curr;

    	while (curr != NULL && (curr->vir_base < new_region->vir_base)) {
        	prev = curr;
        	curr = curr->next;
    	}
    	prev->next = new_region;
    	new_region->next = curr;

	}else{
		as->regionList = new_region;
	}
}

int
as_prepare_load(struct addrspace *as)
{
    /*
     * Write this.
     */
	//Add****************
	struct region* curr = as->regionList;
	while(curr != NULL){
		curr->prev_writeable = curr->writeable;
		curr->writeable = 1;
		curr = curr->next;
	}
	//******************

    //(void)as;
    return 0;
}

int
as_complete_load(struct addrspace *as)
{
    /*
     * Write this.
     */


	//Add*****************
	struct region* curr = as->regionList;
	while(curr != NULL){
		curr->writeable = curr->prev_writeable;
		curr = curr->next;
	}

	int s = splhigh();

	//Flush the tlb
	for (int i = 0; i< NUM_TLB; i++){
		tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}
	
	splx(s);

	//********************
    return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
    /*
     * Write this.
     */	
	
	int res = as_define_region(as, USERSTACK - (STACK_SIZE_IN_PAGE * PAGE_SIZE), 
								(STACK_SIZE_IN_PAGE * PAGE_SIZE), 1, 1, 1);
	
	if(res){
		return res;
	}

    /* Initial user-level stack pointer */
    *stackptr = USERSTACK;

    return 0;
}

struct region* region_create(vaddr_t vaddr, size_t num_of_pages, int readable,
                                   int writeable, int executable)
{
	struct region* new_region = kmalloc(sizeof(struct region));
	
	if (new_region == NULL) {
    	return 0;
	}
	
	new_region->vir_base = vaddr;
	new_region->num_of_pages = num_of_pages;
	new_region->readable = readable;
	new_region->writeable = writeable;
	new_region->executable = executable;
	new_region->prev_writeable = new_region->writeable;
	new_region->next = NULL;

	return new_region;
}

void region_destroy(struct addrspace* as, struct region* region)
{
	if(region==NULL) return;
    

    for(unsigned int i=0; i<region->num_of_pages; ++i) {
    	// Find VPN
    	vaddr_t VPN = region->vir_base & PAGE_FRAME;
    	VPN += i*PAGE_SIZE;
    	
    	// Find the coresponding hpt_entry
        struct hpt_entry * curr_hpt_entry = hpt_lookup(as, VPN);

        if(curr_hpt_entry != NULL) {
            // Get PFN, get rid of N,D,V,G bits
            paddr_t PFN = curr_hpt_entry->PFN & PAGE_FRAME;
            
            // free physical frame
            kfree((void *)PADDR_TO_KVADDR(PFN));

            // delete corresponding entry in hash_page_table
            hpt_delete(as, VPN);
        }
    }

    kfree(region);
    KASSERT(region==NULL);
}