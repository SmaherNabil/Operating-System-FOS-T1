#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"

int initialize_kheap_dynamic_allocator(uint32 daStart,
		uint32 initSizeToAllocate, uint32 daLimit) {
	//TODO: [PROJECT'23.MS2 - #01] [1] KERNEL HEAP - initialize_kheap_dynamic_allocator()
	//Initialize the dynamic allocator of kernel heap with the given start address, size & limit
	//All pages in the given range should be allocated
	//Remember: call the initialize_dynamic_allocator(..) to complete the initialization
	//Return:
	//	On success: 0
	//	Otherwise (if no memory OR initial size exceed the given limit): E_NO_MEM
	// Initialize the kernel heap dynamic allocator
	da_Start = daStart;
	da_Limit = daLimit;
	segmentBreak = ROUNDUP(initSizeToAllocate + daStart, PAGE_SIZE);
	if (initSizeToAllocate > daLimit) {
		return E_NO_MEM;
	}
	//assuming that daLimit can be after sbrk
	struct FrameInfo *allocPtr = NULL;
	//allocate and map each page
	for (uint32 i = da_Start; i < da_Limit; i += PAGE_SIZE) {
		//int ret = allocate_frame(&allocPtr);
		//if no free frame in Kernel or wants to initialize size greater than limit
		if (allocate_frame(&allocPtr) != 0) {
			//this function is responsible for
			//bringing a physical frame and storing information about it
			//failed to allocate memory for a frame
			return E_NO_MEM;
		}

		//if the frame allocation is successful, the code proceeds to map the
		//allocated frame to the virtual address i(elly ana 3andaha now)
		if (map_frame(ptr_page_directory, allocPtr, i,
				PERM_PRESENT | PERM_WRITEABLE) != 0) {
			//failed to map the frame to the virtual address
			return E_NO_MEM;
		}
		allocPtr->va = i;

	}
	initialize_dynamic_allocator(daStart, initSizeToAllocate);
	return 0;

}

void* sbrk(int increment) {
	//TODO: [PROJECT'23.MS2 - #02] [1] KERNEL HEAP - sbrk()
	/* increment > 0: move the segment break of the kernel to increase the size of its heap,
	 * 				you should allocate pages and map them into the kernel virtual address space as necessary,
	 * 				and returns the address of the previous break (i.e. the beginning of newly mapped memory).
	 * increment = 0: just return the current position of the segment break
	 * increment < 0: move the segment break of the kernel to decrease the size of its heap,
	 * 				you should deallocate pages that no longer contain part of the heap as necessary.
	 * 				and returns the address of the new break (i.e. the end of the current heap space).
	 *
	 * NOTES:
	 * 	1) You should only have to allocate or deallocate pages if the segment break crosses a page boundary
	 * 	2) New segment break should be aligned on page-boundary to avoid "No Man's Land" problem
	 * 	3) Allocating additional pages for a kernel dynamic allocator will fail if the free frames are exhausted
	 * 		or the break exceed the limit of the dynamic allocator. If sbrk fails, kernel should panic(...)
	 */
	uint32 oldSegBrk = segmentBreak;
	uint32 localSegBrk = oldSegBrk;
	localSegBrk = ROUNDUP(oldSegBrk + increment, PAGE_SIZE);
	int numOfPages = (localSegBrk - oldSegBrk) / PAGE_SIZE;
	struct FrameInfo *allocPtr = NULL;
	if (increment == 0) {
		//return ((void*)oldSegBrk-1);
		return (void*) oldSegBrk;
	} else if (increment > 0) {
		if (increment + segmentBreak > da_Limit) {
			panic("Size greater than limit!");
		}

		for (int i = 0; i < numOfPages; i++) {
			int ret = allocate_frame(&allocPtr);
			//if no free frame in Kernel or wants to initialize size greater than limit
			if (ret != E_NO_MEM) {
				map_frame(ptr_page_directory, allocPtr, segmentBreak,
						PERM_PRESENT | PERM_WRITEABLE);
			} else if (ret == E_NO_MEM) {
				panic("Free frames are exhausted!");
			}
			allocPtr->va = segmentBreak;
			segmentBreak += PAGE_SIZE;
			//allocPtr->va=segmentBreak;
		}
		return (void*) oldSegBrk;
	} else if (increment < 0) {
		//if the increment is lower than the start
		if (segmentBreak + increment < da_Start) {
			panic("Can't increment before the start of the heap :) ");
		}

			else {
			for(uint32 va =localSegBrk; va < oldSegBrk; va += PAGE_SIZE) {
				unmap_frame(ptr_page_directory, segmentBreak);
				}
			segmentBreak = segmentBreak +increment ;
			return (void *) segmentBreak;
		}

	}
	//MS2: COMMENT THIS LINE BEFORE START CODING====
	return (void*) -1;

}

#define Num_Of_Pages ((KERNEL_HEAP_MAX - KERNEL_HEAP_START)) / PAGE_SIZE
struct VirtAddressSize {
	uint32 address;
	uint32 pagesno;
	uint32 mysize;
	uint32 end_virt_add;
};
struct VirtAddressSize kfreeAddress[Num_Of_Pages] = { 0 };
int numAllocations = -1;

void* kmalloc(unsigned int size) {
	//TODO: [PROJECT'23.MS2 - #03] [1] KERNEL HEAP - kmalloc()
	//refer to the project presentation and documentation for details
	// use "isKHeapPlacementStrategyFIRSTFIT() ..." functions to check the current strategy
	//change this "return" according to your answer
	//kpanic_into_prompt("kmalloc() is not implemented yet...!!");
	uint32 desired_pages = ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE;
	uint32 start_address = da_Limit + PAGE_SIZE;
	uint32 *Page_Table_Ptr = NULL;
	int pageCounter = 0;
	struct FrameInfo* ptr_frame_info;
	uint32 currentAddress = 0;
	uint32 index=0;

	// Check if the requested size exceeds the available space
	if (start_address + (desired_pages*PAGE_SIZE) > KERNEL_HEAP_MAX) {
		return NULL;
	}

	if (size <= DYN_ALLOC_MAX_BLOCK_SIZE) {
		// void*  ptr = alloc_block_FF(size);
		return (void*)alloc_block_FF(size);
	}
	else if(size > DYN_ALLOC_MAX_BLOCK_SIZE) {
		if (isKHeapPlacementStrategyFIRSTFIT() == 1) {
			// Loop over the kernel heap address space
			//cprintf("this is our faulty address %x",start_address);
			for (uint32 i = start_address; i < KERNEL_HEAP_MAX; i +=PAGE_SIZE)
			{
				//struct FrameInfo* ptr_frame_info;
				ptr_frame_info = get_frame_info(ptr_page_directory, i,
						&Page_Table_Ptr);
				if (ptr_frame_info == 0) {
					// Found a free frame
					if (pageCounter == 0) {
						// Start of a potential consecutive block
						currentAddress = i;
						index = i;
					}
					pageCounter++;
					if (pageCounter == desired_pages) {
						break;
					}
				} else if (ptr_frame_info != NULL) {
					// End of the free segment
					pageCounter = 0;
				}
			}
		} else {
			// Return NULL if the placement strategy is not FIRSTFIT
			return NULL;
		}
	}

	if (pageCounter == desired_pages) {
		// Allocate and map the required memory pages
		for (uint32 i = 0; i < desired_pages; i++) {
			allocate_frame(&ptr_frame_info);
			int perm = PERM_PRESENT | PERM_WRITEABLE;
			map_frame(ptr_page_directory, ptr_frame_info, currentAddress, perm);
			ptr_frame_info->va = currentAddress;
			currentAddress += PAGE_SIZE;
		}
		uint32 da = (index - KERNEL_HEAP_START) / PAGE_SIZE;
		// Store the allocation information to be used in the kfree()
		kfreeAddress[da].address = index;
		kfreeAddress[da].pagesno = desired_pages;
		kfreeAddress[da].mysize = desired_pages * PAGE_SIZE;
		kfreeAddress[da].end_virt_add = currentAddress;

	} else if (pageCounter < desired_pages) {
		return NULL;
	}

	// Return the allocated memory address
	return (void*) (index);
}

void kfree(void* virtual_address) {
	//TODO: [PROJECT'23.MS2 - #04] [1] KERNEL HEAP - kfree()
	uint32 CurrentAddress = (uint32)virtual_address;
	uint32 start_address = da_Limit + PAGE_SIZE;
	uint32 HeapStart;
	if (CurrentAddress >= KERNEL_HEAP_START && CurrentAddress < da_Limit) {
		free_block(virtual_address);
	}
	else if (CurrentAddress>= start_address&& CurrentAddress < KERNEL_HEAP_MAX) {
		// Free and unmap the associated memory pages
		uint32 da = (CurrentAddress - KERNEL_HEAP_START) / PAGE_SIZE;
		uint32 pagesToFree = kfreeAddress[da].pagesno;
		for (int i = 0; i < pagesToFree; i++) {
			da = (CurrentAddress - KERNEL_HEAP_START) / PAGE_SIZE;
			//cprintf("gowa el loop%x\n", allocationIndex);
			// Unmap the page
			unmap_frame(ptr_page_directory, CurrentAddress);
			kfreeAddress[da].mysize = 0;
			kfreeAddress[da].pagesno = 0;
			kfreeAddress[da].address = 0;
			kfreeAddress[da].end_virt_add = 0;
			CurrentAddress += PAGE_SIZE;
		}
	}
	else {
		panic("wrong address!");
	}
}

unsigned int kheap_virtual_address(unsigned int physical_address) {
	//TODO: [PROJECT'23.MS2 - #05] [1] KERNEL HEAP - kheap_virtual_address()
	//refer to the project presentation and documentation for details
	// Write your code here, remove the panic and write your code
	//panic("kheap_virtual_address() is not implemented yet...!!");
	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================

	struct FrameInfo *to_va_addrs = NULL;
	to_va_addrs = to_frame_info(physical_address);
	if (to_va_addrs->references != 0) {
		uint32 mypointer = to_va_addrs->va + (physical_address & 0x00000FFF);
		return mypointer;
	}

	return 0;
}

unsigned int kheap_physical_address(unsigned int virtual_address) {
	//TODO: [PROJECT'23.MS2 - #06] [1] KERNEL HEAP - kheap_physical_address()
	//refer to the project presentation and documentation for details
	// Write your code here, remove the panic and write your code
	//panic("kheap_physical_address() is not implemented yet...!!");
	uint32* ptr_page_table = NULL;
	uint32 To_phy_addrs;
	get_page_table(ptr_page_directory, virtual_address, &ptr_page_table);
	To_phy_addrs = ((ptr_page_table[PTX(virtual_address)] >> 12) * PAGE_SIZE)
			+ (virtual_address & 0x00000FFF);
	return To_phy_addrs;
	//change this "return" according to your answer
	//return 0;

}

void kfreeall() {
	panic("Not implemented!");

}

void kshrink(uint32 newSize) {
	panic("Not implemented!");
}

void kexpand(uint32 newSize) {
	panic("Not implemented!");
}

//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// krealloc():

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to kmalloc().
//	A call with new_size = zero is equivalent to kfree().

#define MAXNO KERNEL_HEAP_MAX - (da_Limit + PAGE_SIZE)
void *krealloc(void *virtual_address, uint32 new_size) {
	//TODO: [PROJECT'23.MS2 - BONUS#1] [1] KERNEL HEAP - krealloc()
	// Write your code here, remove the panic and write your code
	//return NULL;
	//panic("krealloc() is not implemented yet...!!");
	uint32 desired_pages = ROUNDUP(new_size, PAGE_SIZE) / PAGE_SIZE;
	int counter = 0;
	uint32 index=0;
	//max num of pages
	//initially set to worst difference possible
	uint32 bestDiff = MAXNO/PAGE_SIZE;
	uint32 currDiff=0;
	uint32 bestIndex=-1;
	uint32 currentAddress = 0;
	uint32 start_address = da_Limit + PAGE_SIZE;
	uint32 *Page_Table_Ptr = NULL;
	struct FrameInfo* ptr_frame_info;

	// Check if the requested size exceeds the available space
	if (start_address + (desired_pages*PAGE_SIZE) > KERNEL_HEAP_MAX) {
		return NULL;
	}
	// nbda2 el shoghl ;)
	if(virtual_address == NULL && new_size != 0){
		return kmalloc(new_size);
	}
	else if (new_size == 0 && virtual_address != NULL){
		kfree(virtual_address);
		return NULL;
	}
	else{
		if (new_size <= DYN_ALLOC_MAX_BLOCK_SIZE){
			kfree(virtual_address);
			return (void*)alloc_block_BF(new_size);
			/*should we return alloc block BF or realloc ff?*/
		}
		else if(new_size > DYN_ALLOC_MAX_BLOCK_SIZE){
			if (isKHeapPlacementStrategyBESTFIT() == 1) {

				//loop over RAM and which 'gap' is closest to our size
				for (uint32 i = start_address; i < KERNEL_HEAP_MAX; i +=PAGE_SIZE){
					ptr_frame_info = get_frame_info(ptr_page_directory, i,&Page_Table_Ptr);
					//found a free frame
					if (ptr_frame_info == 0) {
						if (counter == 0) {
							// Start of a potential consecutive block
							currentAddress = i;
							index = i;
						}
						counter++;
						//currDiff = counter-desired_pages;

						if (counter >= desired_pages){//&& currDiff < bestDiff) {
							currDiff = counter-desired_pages;
						}
					}
					//reset counter
					else if (ptr_frame_info != NULL) {
						if(currDiff < bestDiff){
							bestIndex = index;
							bestDiff = currDiff;
						}
						counter=0;
					}
				}
				//in case of failure (no place un RAM)
				if(bestIndex==-1){
					//old va remains vaild --> won't use kfree
					return NULL;
				}
			}
			else {
				// Return NULL if the placement strategy is not BESTFIT
				return NULL;
			}
		}
	}
//	kfree(virtual_address);

	// Allocate and map the required memory pages
	uint32 tempInd = bestIndex;
	struct FrameInfo * finalFrame;
	finalFrame = get_frame_info(ptr_page_directory, bestIndex,&Page_Table_Ptr);

	for (int i = 0; i < desired_pages; i++) {
	    allocate_frame(&finalFrame);
		int perm = PERM_PRESENT | PERM_WRITEABLE;
		map_frame(ptr_page_directory, finalFrame, tempInd, perm);
		finalFrame->va = tempInd;
		tempInd += PAGE_SIZE;
		}

	int da = (bestIndex - KERNEL_HEAP_START) / PAGE_SIZE;
	// Store the allocation information to be used in the kfree()
	kfreeAddress[da].address = bestIndex;
	kfreeAddress[da].pagesno = desired_pages;
	kfreeAddress[da].mysize = desired_pages * PAGE_SIZE;
	kfreeAddress[da].end_virt_add = tempInd;

	// Return the allocated memory address
	return (void*) (bestIndex);
}


