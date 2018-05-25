#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <addrspace.h>
#include <vm.h>
#include <synch.h>

/* Place your frametable data-structures here 
 * You probably also want to write a frametable initialisation
 * function and call it from vm_bootstrap
 */


//Add***********************************
//The struct of frame table entry
struct ft_entry{
	int prev;
	int next;
	bool used;
};
////***********************************



static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;

//Add********************************
//Frame table
struct ft_entry* frameTable = NULL;   
//********************************8


/* Note that this function returns a VIRTUAL address, not a physical 
 * address
 * WARNING: this function gets called very early, before
 * vm_bootstrap().  You may wish to modify main.c to call your
 * frame table initialisation function, or check to see if the
 * frame table has been initialised and call ram_stealmem() otherwise.
 */

vaddr_t alloc_kpages(unsigned int npages)
{
        /*
         * IMPLEMENT ME.  You should replace this code with a proper
         *                implementation.
         */

        paddr_t addr;

        spinlock_acquire(&stealmem_lock);
        addr = ram_stealmem(npages);
        spinlock_release(&stealmem_lock);

        if(addr == 0)
                return 0;

        return PADDR_TO_KVADDR(addr);
}

void free_kpages(vaddr_t addr)
{
        (void) addr;
}

//Add***********************
//Frametable initialization
void init_frametable(){

		//paddr_t top_of_ram = ram_getsize();
		paddr_t top_ram = ram_getsize();
		//Get the number of frames
		
		unsigned int size_of_frames = (top_ram)/PAGE_SIZE;
		paddr_t bound = top_ram - (size_of_frames * sizeof(struct ft_entry));

		//Frame table		
		frameTable = (struct ft_entry* ) PADDR_TO_KVADDR(bound);

		//Create frame table entries
		struct ft_entry ft_e;
		for(unsigned int i = 0; i < size_of_frames; i++){
			
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

			ft_e.used = false;
		
			memmove(&frameTable[i], &ft_e, sizeof(ft_e));
		}


		paddr_t paddr_firstfree = ram_getfirstfree();
		unsigned int frame_firstfree = paddr_firstfree >> 12;
		//Remove the frames used for kernel from free list				
		for(unsigned int i = 0; i <= frame_firstfree+1; i++){
			frameTable[frameTable[i].next].prev = frameTable[i].prev;
			frameTable[frameTable[i].prev].next = frameTable[i].next;
			frameTable[i].used = true;			
		}
		
		//Remove the frames used for frame trable from free list
		unsigned int frame_bound = bound >> 12;
		for(unsigned int i = size_of_frames-1; i>= frame_bound; i--){
			frameTable[frameTable[i].next].prev = frameTable[i].prev;
			frameTable[frameTable[i].prev].next = frameTable[i].next;
			frameTable[i].used = true;					
		}

}
//**************************
