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
#include <synch.h>
//

/* Place your page table functions here */

// Lock for hpt
static struct spinlock hpt_lock = SPINLOCK_INITIALIZER;

void init_hpt(void)
{
	paddr_t top_of_ram = ram_getsize();
	
	// Get the number of frames		
	int num_of_frames = (top_of_ram)/PAGE_SIZE;
	
	// Using externel chain, don't need allocate two times of slots
	hpt_size = num_of_frames;

	// Bump allocator will be used
	hash_page_table = (struct hpt_entry**) kmalloc(sizeof(struct hpt_entry*)*hpt_size);
	KASSERT(hash_page_table!=NULL);

	for(int i=0; i<hpt_size; ++i)
	{
		// pointing to null, no head yet
		hash_page_table[i] = NULL;
	}
}

uint32_t hpt_hash(struct addrspace *as, vaddr_t faultaddr)
{
    uint32_t index;

    index = (((uint32_t )as) ^ (faultaddr >> PAGE_BITS)) % hpt_size;
    return index;
}

// Insert into [index] linked list
struct hpt_entry* hpt_insert(struct addrspace * as, vaddr_t VPN, paddr_t PFN, 
									int n_bit, int d_bit, int v_bit)
{
    if(n_bit > 0) {
        PFN = PFN | TLBLO_NOCACHE; 
    }
    if(d_bit > 0) {
        PFN = PFN | TLBLO_DIRTY;
    }
    if(v_bit > 0) {
        PFN = PFN | TLBLO_VALID;
    }
	
    uint32_t index = hpt_hash(as, VPN);

    // Get the head of [index] linked list
	struct hpt_entry * new_hpt_entry = hash_page_table[index];

	spinlock_acquire(&hpt_lock);

	// No element in hash_page_table[index] 
	if(new_hpt_entry==NULL)
	{
		new_hpt_entry = kmalloc(sizeof(struct hpt_entry));
		KASSERT(new_hpt_entry!=NULL);
		new_hpt_entry->pid = as;
		new_hpt_entry->VPN = VPN;
		new_hpt_entry->PFN = PFN;
		new_hpt_entry->next = NULL;
		hash_page_table[index] = new_hpt_entry;
		spinlock_release(&hpt_lock);
		return new_hpt_entry;
	}

	while(new_hpt_entry->next!=NULL)
	{
		new_hpt_entry = new_hpt_entry->next;
	}

	struct hpt_entry * tail_hpt_entry = new_hpt_entry;

	new_hpt_entry = kmalloc(sizeof(struct hpt_entry));
	KASSERT(new_hpt_entry!=NULL);
	new_hpt_entry->pid = as;
	new_hpt_entry->VPN = VPN;
	new_hpt_entry->PFN = PFN;
	new_hpt_entry->next = NULL;
	tail_hpt_entry->next = new_hpt_entry;
	
	spinlock_release(&hpt_lock);
	
	return new_hpt_entry;
}

int hpt_delete(struct addrspace * as, vaddr_t VPN)
{
    uint32_t index = hpt_hash(as, VPN);

	struct hpt_entry * curr = hash_page_table[index];

	struct hpt_entry * prev = NULL;

	spinlock_acquire(&hpt_lock);

	// if the to be deleted one is the head
	if(curr->pid==as && curr->VPN==VPN)
	{
		hash_page_table[index]=curr->next;
		kfree(curr);
		spinlock_release(&hpt_lock);
		return 0;
	}

	while(curr->next!=NULL)
	{
		prev = curr;
		curr = curr->next;
		if(curr->pid==as && curr->VPN==VPN)
		{
			prev->next=curr->next;
			kfree(curr);
			spinlock_release(&hpt_lock);
			return 0;
		}
	}
	return 0;
}

// Find if there is a hpt_entry based on as and VPN
struct hpt_entry * hpt_lookup(struct addrspace * as, vaddr_t VPN) 
{
    uint32_t index = hpt_hash(as, VPN);

    struct hpt_entry * curr = hash_page_table[index];
    
    spinlock_acquire(&hpt_lock);

    while(curr != NULL) 
    {
        if(curr->pid==as && curr->VPN==VPN) 
        {
            if( (curr->PFN&TLBLO_VALID) == TLBLO_VALID) 
            {
                spinlock_release(&hpt_lock);
                return curr;
            } 
        }
        curr = curr->next;
    }

    spinlock_release(&hpt_lock);
    return NULL;
}

void vm_bootstrap(void)
{
        /* Initialise VM sub-system.  You probably want to initialise your 
           frame table here as well.
        */
		

		// init hpt first, so will use bump allocator
		init_hpt();

		init_frametable();

}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
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

	struct addrspace *curr_as = proc_getas();

	if(curr_as == NULL){
		return EFAULT;
	}

	// Lookup hpt
	vaddr_t old_VPN = faultaddress&PAGE_FRAME;
	struct hpt_entry* old_hpt_entry = hpt_lookup(curr_as, old_VPN);
	
	// Load TLB if valid
	if(old_hpt_entry!=NULL)
	{
		int s = splhigh();
		tlb_random(old_hpt_entry->VPN, old_hpt_entry->PFN);
		splx(s);
		return 0;
	}

	// Lookup regions
    struct region* curr = curr_as->regionList;
    if(curr == NULL) 
    {
        return EFAULT;
    }

    while(curr != NULL) 
    {
        if (((curr->vir_base + curr->num_of_pages*PAGE_SIZE) > faultaddress) 
        				&& (curr->vir_base <= faultaddress)) 
        {
            break;
        }
        curr = curr->next;
    }

    // No valid region
    if(curr==NULL)
    {
        return EFAULT;
    }

	// Get a frame in frameTable
	vaddr_t VPN = (vaddr_t) kmalloc(PAGE_SIZE);
    if(VPN == 0) {
        return ENOMEM;
    }

	VPN &= TLBHI_VPAGE;

	// Convert to physical address
	paddr_t PFN = KVADDR_TO_PADDR(VPN);
	PFN &= TLBLO_PPAGE;

	// Create a new hpt_entry and insert into hpt
	struct hpt_entry* new_hpt_entry = 
					hpt_insert(curr_as, old_VPN, PFN, 0, curr->writeable, 1);

	if(new_hpt_entry == NULL){
		return ENOMEM;
	}

	// load TLB
	int t = splhigh();
	tlb_random(new_hpt_entry->VPN, new_hpt_entry->PFN);
	splx(t);
	
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

