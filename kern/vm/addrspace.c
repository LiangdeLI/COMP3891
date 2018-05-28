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

		//Add*****************************
		as->pageTable = (paddr_t **)alloc_kpages(1);
		if(as->pageTable == NULL){
			kfree(as);
			return NULL;		
		}

		for(int i = 0; i < SIZE_OF_PAGETABLE; i++){
			as->pageTable[i] = NULL;
		}

		//as->regions = NULL;		
		//********************************

        return as;
}

int
as_copy(struct addrspace *old, struct addrspace **ret)
{
        struct addrspace *newas;

        newas = as_create();
        if (newas==NULL) {
                return ENOMEM;
        }

        //Add***************** Not finished
        struct region* old_region = old->regionList;
		struct region* new_region = newas->regionList;

		while(old_region != NULL){
			struct region* reg = kmalloc(sizeof(struct region));
			if(reg == NULL){
				//as_destroy();
				return ENOMEM;			
			}else{
				reg->vir_base = old_region -> vir_base;
				reg->size_of_frames = old_region->size_of_frames;
				reg->w_bit = old_region->w_bit;
				reg->next = NULL;

				if(new_region == NULL){
					newas->regionList = reg;
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
			newas->pageTable[i] = kmalloc(sizeof(paddr_t) * SIZE_OF_PAGETABLE);
			for(int j = 0; j < SIZE_OF_PAGETABLE; j++){

				if(old->pageTable[i][j] == 0){
					newas->pageTable = 0;				
				}else{
					vaddr_t frame_addr_new = alloc_kpages(1);
					//vaddr_t frame_addr_old = PADDR_TO_KVADDR(old->pageTable[i][j] & PAGE_FRAME)
					memmove((void*)frame_addr_new, (const void *)PADDR_TO_KVADDR(old->pageTable[i][j] & PAGE_FRAME), PAGE_SIZE);
					//dirty = 
					newas->pageTable[i][j] = (PAGE_FRAME & KVADDR_TO_PADDR(frame_addr_new)) | TLBLO_VALID | (old->pageTable[i][j] & TLBLO_DIRTY);
				}
				
			}
		}


        //*******************

        //(void)old;

        *ret = newas;
        return 0;
}

void
as_destroy(struct addrspace *as)
{
        /*
         * Clean up as needed.
         */

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
}

void
as_deactivate(void)
{
        /*
         * Write this. For many designs it won't need to actually do
         * anything. See proc.c for an explanation of why it (might)
         * be needed.
         */
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

        (void)as;
        (void)vaddr;
        (void)memsize;
        (void)readable;
        (void)writeable;
        (void)executable;
        return ENOSYS; /* Unimplemented */
}

int
as_prepare_load(struct addrspace *as)
{
        /*
         * Write this.
         */

        (void)as;
        return 0;
}

int
as_complete_load(struct addrspace *as)
{
        /*
         * Write this.
         */

        (void)as;
        return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
        /*
         * Write this.
         */

        (void)as;

        /* Initial user-level stack pointer */
        *stackptr = USERSTACK;

        return 0;
}

