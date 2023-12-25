#include <inc/lib.h>

//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//
#define Num_Of_Pages ((USER_HEAP_MAX - USER_HEAP_START)) / PAGE_SIZE

int FirstTimeFlag = 1;
void InitializeUHeap() {
	if (FirstTimeFlag) {
#if UHP_USE_BUDDY
		initialize_buddy();
		cprintf("BUDDY SYSTEM IS INITIALIZED\n");
#endif
		FirstTimeFlag = 0;
	}
}

struct userAddresses {
	uint32 va;
	int size;
	bool reserved;
};
struct userAddresses ListUseradd[Num_Of_Pages] = { 0 };
//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//=============================================
// [1] CHANGE THE BREAK LIMIT OF THE USER HEAP:
//=============================================
/*2023*/
void* sbrk(int increment) {
	return (void*) sys_sbrk(increment);
}
//=================================
// [2] ALLOCATE SPACE IN USER HEAP:
//=================================
void* malloc(uint32 size) {
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	if (size == 0)
		return NULL;
	//==============================================================
	//TODO: [PROJECT'23.MS2 - #09] [2] USER HEAP - malloc() [User Side]
	// Write your code here, remove the panic and write your code

	uint32 start_address = USER_HEAP_START + DYN_ALLOC_MAX_SIZE + PAGE_SIZE;
	int desired_pages = ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE;
	int pageCounter = 0;
	uint32 currentAddress = 0;
	uint32 index = 0;

//	if (start_address + (desired_pages) > USER_HEAP_MAX) {
//					return NULL;
//	}

	//block allocator
	if (size <= DYN_ALLOC_MAX_BLOCK_SIZE) {
		//cprintf("before calling alloc ff in malloc \n ");
		void* ret = alloc_block_FF(size);
//		cprintf("return of ff is = %x", ret);
		return ret;
	}
	//page allocator
	else if (size > DYN_ALLOC_MAX_BLOCK_SIZE) {
		if (sys_isUHeapPlacementStrategyFIRSTFIT() == 1) {
			// Loop over the kernel heap address space
			//cprintf("this is our faulty address %x",start_address);
			for (uint32 i = start_address; i < USER_HEAP_MAX; i += PAGE_SIZE)
			{
				if (ListUseradd[(i - USER_HEAP_START) / PAGE_SIZE].reserved
						== 0) {
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
				} else {
					// End of the free segment
					pageCounter = 0;
					i =i+ ListUseradd[(i - USER_HEAP_START)/ PAGE_SIZE].size - PAGE_SIZE;
				}
			}
		} else {
			// Return NULL if the placement strategy is not FIRSTFIT
			return NULL;
		}
	}
	if (pageCounter == desired_pages) {
		uint32 da = (index - USER_HEAP_START) / PAGE_SIZE;
		sys_allocate_user_mem(currentAddress, (desired_pages * PAGE_SIZE));
		ListUseradd[da].va = index;
		ListUseradd[da].size = desired_pages * PAGE_SIZE;
		ListUseradd[da].reserved = 1;
		return (void*) (index);
	} else if (pageCounter < desired_pages) {
		return NULL;
	}
	//Use sys_isUHeapPlacementStrategyFIRSTFIT() and
	//sys_isUHeapPlacementStrategyBESTFIT()
	//to check the current strategy
	// Return the allocated memory address

	return (void*) (index);
	//Use sys_isUHeapPlacementStrategyFIRSTFIT() and	sys_isUHeapPlacementStrategyBESTFIT()
	//to check the current strategy
}
//=================================
// [3] FREE SPACE FROM USER HEAP:
//=================================
void free(void* virtual_address) {
	//TODO: [PROJECT'23.MS2 - #11] [2] USER HEAP - free() [User Side]
	// Write your code here, remove the panic and write your code
	//panic("free() is not implemented yet...!!");
	uint32 CurrentAddress = (uint32) virtual_address;
	//USER_HEAP_START+DYN_ALLOC_MAX_SIZE
	uint32 startAdd = sys_get_hard_limit() + PAGE_SIZE;

	if (CurrentAddress >= USER_HEAP_START
			&& CurrentAddress < sys_get_hard_limit()) {
		free_block(virtual_address);
	} else {
		int index = (CurrentAddress - USER_HEAP_START) / PAGE_SIZE;
		int pages_needed = (ListUseradd[index].size / PAGE_SIZE);

		sys_free_user_mem(CurrentAddress, ListUseradd[index].size);
		//update list
		ListUseradd[index].reserved = 0;
		ListUseradd[index].size = 0;

	}
}

//=================================
// [4] ALLOCATE SHARED VARIABLE:
//=================================
void* smalloc(char *sharedVarName, uint32 size, uint8 isWritable) {
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	if (size == 0)
		return NULL;
	//==============================================================
	panic("smalloc() is not implemented yet...!!");
	return NULL;
}

//========================================
// [5] SHARE ON ALLOCATED SHARED VARIABLE:
//========================================
void* sget(int32 ownerEnvID, char *sharedVarName) {
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	//==============================================================
	// Write your code here, remove the panic and write your code
	panic("sget() is not implemented yet...!!");
	return NULL;
}

//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//

//=================================
// REALLOC USER SPACE:
//=================================
//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to malloc().
//	A call with new_size = zero is equivalent to free().

//  Hint: you may need to use the sys_move_user_mem(...)
//		which switches to the kernel mode, calls move_user_mem(...)
//		in "kern/mem/chunk_operations.c", then switch back to the user mode here
//	the move_user_mem() function is empty, make sure to implement it.
void *realloc(void *virtual_address, uint32 new_size) {
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	//==============================================================

	// Write your code here, remove the panic and write your code
	panic("realloc() is not implemented yet...!!");
	return NULL;

}

//=================================
// FREE SHARED VARIABLE:
//=================================
//	This function frees the shared variable at the given virtual_address
//	To do this, we need to switch to the kernel, free the pages AND "EMPTY" PAGE TABLES
//	from main memory then switch back to the user again.
//
//	use sys_freeSharedObject(...); which switches to the kernel mode,
//	calls freeSharedObject(...) in "shared_memory_manager.c", then switch back to the user mode here
//	the freeSharedObject() function is empty, make sure to implement it.

void sfree(void* virtual_address) {
	// Write your code here, remove the panic and write your code
	panic("sfree() is not implemented yet...!!");
}

//==================================================================================//
//========================== MODIFICATION FUNCTIONS ================================//
//==================================================================================//

void expand(uint32 newSize) {
	panic("Not Implemented");

}
void shrink(uint32 newSize) {
	panic("Not Implemented");

}
void freeHeap(void* virtual_address) {
	panic("Not Implemented");

}
