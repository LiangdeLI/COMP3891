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


//Frame table
struct ft_entry* frameTable = NULL;  

static int first_free_index;

static int top_of_bump_allocated;

static struct spinlock frameTable_lock = SPINLOCK_INITIALIZER;

//***********************************



static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;




/* Note that this function returns a VIRTUAL address, not a physical 
 * address
 * WARNING: this function gets called very early, before
 * vm_bootstrap().  You may wish to modify main.c to call your
 * frame table initialisation function, or check to see if the
 * frame table has been initialised and call ram_stealmem() otherwise.
 */

vaddr_t alloc_kpages(unsigned int npages)
{
        //***************
		
		//******
		paddr_t addr = 0; //Initialization to 0
		spinlock_acquire(&frameTable_lock);     
		
		if(frameTable != NULL){
			//we support only one page
			if(npages != 1){
				return 0;
			}
			
			//If there is no more free frame
			if (first_free_index == -1){
				return 0;
			}
			
			addr = first_free_index << 12;
			
			//
			frameTable[frameTable[first_free_index].next].prev = frameTable[first_free_index].prev;
			frameTable[frameTable[first_free_index].prev].next = frameTable[first_free_index].next;
			frameTable[first_free_index].used = true;
			//

			//Check if it is the last free frame
			if(frameTable[first_free_index].next == first_free_index){
				first_free_index = -1;
			}else{
				first_free_index = frameTable[first_free_index].next;
			}

		}else{
			spinlock_acquire(&stealmem_lock);
			addr = ram_stealmem(npages);
        	spinlock_release(&stealmem_lock);				
		}

		spinlock_release(&frameTable_lock);
		//******


        if(addr == 0)
                return 0;
		

		bzero((void*)PADDR_TO_KVADDR(addr),PAGE_SIZE);

        return PADDR_TO_KVADDR(addr);
}

void free_kpages(vaddr_t addr)
{
        //(void) addr;
		//Add**********
		
		paddr_t paddr = KVADDR_TO_PADDR(addr);
		
		int i = paddr >> 12;
		if(i<=top_of_bump_allocated) return;

		spinlock_acquire(&frameTable_lock);

		if(frameTable[i].used){

			if(first_free_index != -1){
				frameTable[i].prev = frameTable[first_free_index].prev;	
				frameTable[first_free_index].prev = i;
				frameTable[i].next = first_free_index;		
			}else{
				frameTable[i].prev = frameTable[i].next = i;
			}
				
			frameTable[i].used = false;

			first_free_index = i;
			spinlock_release(&frameTable_lock);
		}else{
			spinlock_release(&frameTable_lock);
		}
		//*************
}

//Add***********************
//Frametable initialization
void init_frametable(){

		//paddr_t top_of_ram = ram_getsize();
		paddr_t top_of_ram = ram_getsize();
		//Get the number of frames
		
		unsigned int num_of_frames = (top_of_ram)/PAGE_SIZE;
		paddr_t location = top_of_ram - (num_of_frames * sizeof(struct ft_entry));

		//Frame table		
		frameTable = (struct ft_entry*) PADDR_TO_KVADDR(location);

		//Create frame table entries
		struct ft_entry ft_e;
		for(unsigned int i = 0; i < num_of_frames; i++){
			
			if(i == 0){
				ft_e.prev = num_of_frames - 1;				
			}else{
				
				ft_e.prev = i-1;
			}
			
			if(i == num_of_frames-1){
				ft_e.next = 0;
			}else{
				ft_e.next = i+1;
			}

			ft_e.used = false;
			memmove(&frameTable[i], &ft_e, sizeof(ft_e));
		}


		paddr_t paddr_firstfree = ram_getfirstfree();
		unsigned int frame_firstfree = paddr_firstfree >> 12;
		//Remove the frames used for kernel and hpt from free list				
		for(unsigned int i = 0; i <= frame_firstfree; i++){
			frameTable[frameTable[i].next].prev = frameTable[i].prev;
			frameTable[frameTable[i].prev].next = frameTable[i].next;
			frameTable[i].used = true;
			
			first_free_index = i;		
		}
		first_free_index++;
		top_of_bump_allocated = first_free_index-1;
		
		//Remove the frames used for frame table from free list
		unsigned int frame_loc = location >> 12;
		for(unsigned int i = num_of_frames-1; i>= frame_loc; i--){
			frameTable[frameTable[i].next].prev = frameTable[i].prev;
			frameTable[frameTable[i].prev].next = frameTable[i].next;
			frameTable[i].used = true;					
		}

		//kprintf("first_free_index is : 0x%x\n", first_free_index);

}
//**************************
