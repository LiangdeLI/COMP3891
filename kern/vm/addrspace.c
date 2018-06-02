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
    if (new_addr==NULL) 
    {
            return ENOMEM;
    }

    //Add*****************
    struct region* old_region = old->regionList;
	struct region* new_region = new_addr->regionList;		

	while(old_region != NULL)
	{
		struct region* reg = region_copy(new_addr, old, old_region);
		if(reg == NULL)
		{
			//as_destroy();
			return ENOMEM;			
		}

		// If reg is the first region 
		if(new_addr->regionList==NULL)
		{
			new_addr->regionList = reg;
			new_region = reg;
		}
		else
		{
			new_region->next = reg;
			new_region = reg;
		}

		old_region = old_region->next;
	}

    //*******************

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
		prev = cur;
	}

	if(prev!=NULL){
		region_destroy(as, prev);
	}

    kfree(as);
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
	//kprintf("memsize:0x%x\n", memsize);
	//kprintf("vaddr:0x%x\n", vaddr);
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

	//kprintf("as:0x%x define region at 0x%x for 0x%x of pages\n", (unsigned int)as, vaddr, num_of_pages);

	return 0;
	//***************************************
}

void as_add_region(struct addrspace *as, struct region *new_region)
{
	struct region* prev;
	struct region* curr; 

	if(as->regionList != NULL)
	{
		curr = as->regionList;
		prev = curr;

    	while (curr != NULL && (curr->vir_base < new_region->vir_base)) 
    	{
        	prev = curr;
        	curr = curr->next;
    	}
    	
    	if(curr==as->regionList)
    	{
    		new_region->next = curr;
    		as->regionList=new_region;
    	}
    	else
    	{
	    	prev->next = new_region;
	    	new_region->next = curr;
	    }
	}
	else
	{
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
		if(curr->writeable==0)
		{
			curr->need_recover = true;
			curr->writeable = 1;
		}
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
		if(curr->need_recover)
		{
			curr->writeable = 0;
			curr->need_recover=false;
		}
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
	new_region->need_recover = false;
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

}

struct region* region_copy(struct addrspace* new_as, 
						struct addrspace* old, struct region* old_region)
{
	if(old_region==NULL) return NULL;

	struct region* new_region 
			= region_create(old_region->vir_base, 
							old_region->num_of_pages, 
							old_region->readable,
                            old_region->writeable, 
                            old_region->executable);

	for(unsigned int i=0; i<old_region->num_of_pages; ++i)
	{
		struct hpt_entry* old_hpt_entry= hpt_lookup(old, old_region->vir_base+i*PAGE_SIZE);
		
		// No hpt_entry, this page not used yet
		if (old_hpt_entry==NULL) continue;

		// Get a frame in frameTable
		vaddr_t VPN = (vaddr_t) kmalloc(PAGE_SIZE);
		KASSERT(VPN!=0);

		// Convert to virtual address
		paddr_t PFN = KVADDR_TO_PADDR(VPN);
		PFN &= PAGE_FRAME;

		// Get the physical address of old frame
		paddr_t old_PFN = old_hpt_entry->PFN;
		old_PFN &= PAGE_FRAME;

		// Copy the contain of the frame
		memmove((void*)PFN, (const void*)old_PFN, PAGE_SIZE);

		// Create a new hpt_entry and insert into hpt
		hpt_insert(new_as, old_hpt_entry->VPN, PFN, 0, new_region->writeable, 1);

		//kprintf("as:0x%x add page 0x%x at physical 0x%x \n", (unsigned int)new_as, old_hpt_entry->VPN, PFN);
	}

	return new_region;
}
