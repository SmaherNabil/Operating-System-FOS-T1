/*
 * dynamic_allocator.c
 *
 *  Created on: Sep 21, 2023
 *      Author: HP
 */
#include <inc/assert.h>
#include <inc/string.h>
#include "../inc/dynamic_allocator.h"
#define MAXNO 4294967295

//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//=====================================================
// 1) GET BLOCK SIZE (including size of its meta data):
//=====================================================
uint32 get_block_size(void* va) {
	struct BlockMetaData *curBlkMetaData = ((struct BlockMetaData *) va - 1);
	return curBlkMetaData->size;
}

//===========================
// 2) GET BLOCK STATUS:
//===========================
int8 is_free_block(void* va) {
	struct BlockMetaData *curBlkMetaData = ((struct BlockMetaData *) va - 1);
	return curBlkMetaData->is_free;
}

//===========================================
// 3) ALLOCATE BLOCK BASED ON GIVEN STRATEGY:
//===========================================
void *alloc_block(uint32 size, int ALLOC_STRATEGY) {
	void *va = NULL;
	switch (ALLOC_STRATEGY) {
	case DA_FF:
		va = alloc_block_FF(size);
		break;
	case DA_NF:
		va = alloc_block_NF(size);
		break;
	case DA_BF:
		va = alloc_block_BF(size);
		break;
	case DA_WF:
		va = alloc_block_WF(size);
		break;
	default:
		cprintf("Invalid allocation strategy\n");
		break;
	}
	return va;
}

//===========================
// 4) PRINT BLOCKS LIST:
//===========================

void print_blocks_list(struct MemBlock_LIST list) {
	cprintf("=========================================\n");
	struct BlockMetaData* blk;
	cprintf("\nDynAlloc Blocks List:\n");
	LIST_FOREACH(blk, &list)
	{
		cprintf("(size: %d, isFree: %d)\n", blk->size, blk->is_free);
	}
	cprintf("=========================================\n");

}
//
////********************************************************************************//
////********************************************************************************//

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//==================================
// [1] INITIALIZE DYNAMIC ALLOCATOR:
//==================================
struct MemBlock_LIST MemoryList;
struct BlockMetaData *NumOneMetaBlock, *AllocatedBlock, *SplittedBlock;
bool is_initialized = 0;
void initialize_dynamic_allocator(uint32 daStart,uint32 initSizeOfAllocatedSpace) {
	//=========================================
	//DON'T CHANGE THESE LINES=================
	if (initSizeOfAllocatedSpace == 0)
		return;

	is_initialized = 1;

	//=========================================
	//=========================================
	LIST_INIT(&MemoryList);
	NumOneMetaBlock = (struct BlockMetaData *) daStart;
	NumOneMetaBlock->size = initSizeOfAllocatedSpace;
	NumOneMetaBlock->is_free = 1;
	LIST_INSERT_HEAD(&MemoryList, NumOneMetaBlock);

	//TODO: [PROJECT'23.MS1 - #5] [3] DYNAMIC ALLOCATOR - initialize_dynamic_allocator()
	//panic("initialize_dynamic_allocator is not implemented yet");
}

//=========================================
// [4] ALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *alloc_block_FF(uint32 size) {
	//TODO: [PROJECT'23.MS1 - #6] [3] DYNAMIC ALLOCATOR - alloc_block_FF()
	//panic("alloc_block_FF is not implemented yet");
	//condition is done
	if (size == 0) {
		return NULL;
	}

	if (!is_initialized) {
		uint32 required_size = size + sizeOfMetaData();
		uint32 da_start = (uint32) sbrk(required_size);
		//get new break since it's page aligned! thus, the size can be more than the required one
		uint32 da_break = (uint32) sbrk(0);
		initialize_dynamic_allocator(da_start, da_break - da_start);
	}

	//sizeOfAllocatedBlock is the size of the block that i want to search in the heap
	uint32 sizeOfAllocatedBlock = size + sizeOfMetaData();

	//block the new block that i created to put it in the heap
	//SplittedBlock if there is enough space for the meta we should
	//split it and it will point to the free block
	struct BlockMetaData *block, *SplittedBlock;

	LIST_FOREACH(block, &MemoryList)
	{
		if (block->is_free == 1) {
			//there is enough size
			if (sizeOfAllocatedBlock <= block->size) {
				//sizeRemaining the size of the free space after allocating
				uint32 sizeRemaining = block->size - sizeOfAllocatedBlock;

				//can't split because no enough space for one meta data
				if (block->size
						== sizeOfAllocatedBlock||sizeRemaining<sizeOfMetaData()) {
					block->is_free = 0;
					return (void*) ((void*) block + sizeOfMetaData());
				}
				//case that we can split
				else if (sizeRemaining >= sizeOfMetaData()) {
					//the splitted block should point to the new free block
					SplittedBlock = (struct BlockMetaData *) ((uint32) block
							+ sizeOfAllocatedBlock);
					//modifying the new block we created for splitting
					SplittedBlock->is_free = 1;
					SplittedBlock->size = block->size - sizeOfAllocatedBlock;
					//modifying the block that we put the data in
					block->is_free = 0;
					block->size = sizeOfAllocatedBlock;
					LIST_INSERT_AFTER(&MemoryList, block, SplittedBlock);
					return (void*) ((void*) block + sizeOfMetaData());
				}
			}
		}
		else{
			continue;
		}
	}
	//sbrk is called
	int allocSize = size + sizeOfMetaData();
	void * returnValueOfSbrk = sbrk(allocSize);
	struct BlockMetaData* newBlock,* splitted;
	struct BlockMetaData* LastBLock = LIST_LAST(&MemoryList);
	if (returnValueOfSbrk == (void *) -1){
		return NULL;
	}
	else {
		newBlock=returnValueOfSbrk;
		newBlock->size=(uint32)allocSize/*ROUNDUP((uint32)allocSize,PAGE_SIZE)*/;
		int remainingSize=ROUNDUP((uint32)allocSize,PAGE_SIZE)-allocSize;
		if(remainingSize>=sizeOfMetaData())
		{
			splitted = (struct BlockMetaData *) ((uint32) newBlock
										+ allocSize);
			splitted->size=remainingSize;
			splitted->is_free=1;
			newBlock->is_free=0;
			newBlock->size=ROUNDUP(allocSize,PAGE_SIZE)-splitted->size;
			LIST_INSERT_AFTER(&MemoryList, LastBLock, newBlock);
			LIST_INSERT_AFTER(&MemoryList,newBlock,splitted);

		}
		else{
		newBlock->is_free=0;
		newBlock->size=ROUNDUP(allocSize,PAGE_SIZE);
		LIST_INSERT_AFTER(&MemoryList, LastBLock, newBlock);
		}
		return (void*) ((uint32) newBlock + sizeOfMetaData());
	}
	return 	(void*) -1;
}

//=========================================
// [5] ALLOCATE BLOCK BY BEST FIT:
//=========================================
void *alloc_block_BF(uint32 size) {

	// TODO: [PROJECT'23.MS1 - #6] [3] DYNAMIC ALLOCATOR - alloc_block_BF()
	// panic("alloc_block_BF is not implemented yet");

	// Check for invalid size
	if (size == 0) {
		return NULL;
	}

	// Calculate the size of the allocated block including metadata
	uint32 sizeOfAllocatedBlock = size + sizeOfMetaData();
	uint32 bestFitBlockSizeDiff = MAXNO;
	struct BlockMetaData *block = NULL;
	struct BlockMetaData *bestFitBlock = NULL;
	struct BlockMetaData *splitBlock = NULL;

	//iterate over the linked list of blocks
	LIST_FOREACH(block, &MemoryList)
	{
		if (block->is_free && block->size >= sizeOfAllocatedBlock) {
			//calculate the difference between the Heap block size and required size
			uint32 blockSizeDiff = block->size - sizeOfAllocatedBlock;
			//if the current block is a better fit, update the best fit block
			if (blockSizeDiff < bestFitBlockSizeDiff) {
				bestFitBlock = block;
				bestFitBlockSizeDiff = blockSizeDiff;
			}
		}
	}
	if (bestFitBlock != NULL) {
		//sizeRemaining is the size of the free space after allocating
		uint32 sizeRemaining = bestFitBlock->size - sizeOfAllocatedBlock;
		//case1: can't split because no enough space for one metadata
		if (bestFitBlock->size
				== sizeOfAllocatedBlock||sizeRemaining<sizeOfMetaData()) {
			bestFitBlock->is_free = 0;
			return (void*) ((void*) bestFitBlock + sizeOfMetaData());
		}

		//case2: block is larger than or equal to the
		//required size,split it
		else if (bestFitBlock->size - sizeOfAllocatedBlock
				>= sizeof(struct BlockMetaData)) {
			splitBlock = (struct BlockMetaData *) ((uint32) bestFitBlock
					+ sizeOfAllocatedBlock);
			splitBlock->is_free = 1;
			splitBlock->size = bestFitBlock->size - sizeOfAllocatedBlock;
			bestFitBlock->is_free = 0;
			bestFitBlock->size = sizeOfAllocatedBlock;
			LIST_INSERT_AFTER(&MemoryList, bestFitBlock, splitBlock);
		}
		return (void*) ((void*) bestFitBlock + sizeOfMetaData());
	}
	//sbrk is called
		int allocSize = size + sizeOfMetaData();
		void * returnValueOfSbrk = sbrk(allocSize);
		struct BlockMetaData* newBlock,* splitted;
		struct BlockMetaData* LastBLock = LIST_LAST(&MemoryList);
		if (returnValueOfSbrk == (void *) -1){
			return NULL;
		}
		else {
			newBlock=returnValueOfSbrk;
			newBlock->size=(uint32)allocSize/*ROUNDUP((uint32)allocSize,PAGE_SIZE)*/;
			int remainingSize=ROUNDUP((uint32)allocSize,PAGE_SIZE)-allocSize;
			if(remainingSize>=sizeOfMetaData())
			{
				splitted = (struct BlockMetaData *) ((uint32) newBlock
											+ allocSize);
				splitted->size=remainingSize;
				splitted->is_free=1;
				newBlock->is_free=0;
				newBlock->size=ROUNDUP(allocSize,PAGE_SIZE)-splitted->size;
				LIST_INSERT_AFTER(&MemoryList, LastBLock, newBlock);
				LIST_INSERT_AFTER(&MemoryList,newBlock,splitted);

			}
			else{
			newBlock->is_free=0;
			newBlock->size=ROUNDUP(allocSize,PAGE_SIZE);
			LIST_INSERT_AFTER(&MemoryList, LastBLock, newBlock);
			}
			return (void*) ((uint32) newBlock + sizeOfMetaData());
		}
		return (void*)-1;
}

//=========================================
// [6] ALLOCATE BLOCK BY WORST FIT:
//=========================================
void *alloc_block_WF(uint32 size) {
	panic("alloc_block_WF is not implemented yet");
	return NULL;
}

//=========================================
// [7] ALLOCATE BLOCK BY NEXT FIT:
//=========================================
void *alloc_block_NF(uint32 size) {
	panic("alloc_block_NF is not implemented yet");
	return NULL;
}

//===================================================
// [8] FREE BLOCK WITH COALESCING:
//===================================================
void free_block(void *va) {
//TODO: [PROJECT'23.MS1 - #7] [3] DYNAMIC ALLOCATOR - free_block()
//panic("free_block is not implemented yet");
	struct BlockMetaData* toBeFree = ((struct BlockMetaData *) va - 1);
	struct BlockMetaData* ptrToRemove;
//case that virtual address is null should return
	if (toBeFree == NULL) {
		return;
	}
	if (toBeFree->prev_next_info.le_prev != NULL
			&& toBeFree->prev_next_info.le_next != NULL) {
		//case that next and prev are both free
		if (toBeFree->prev_next_info.le_next->is_free == 1
				&& toBeFree->prev_next_info.le_prev->is_free == 1) {
			//modifying prev info
			toBeFree->prev_next_info.le_prev->size =
					toBeFree->prev_next_info.le_prev->size + toBeFree->size
							+ toBeFree->prev_next_info.le_next->size;
			toBeFree->prev_next_info.le_prev->is_free = 1;
			//modifying next and tobefree info
			ptrToRemove = toBeFree->prev_next_info.le_next;
			toBeFree->prev_next_info.le_next->is_free = 0;
			toBeFree->prev_next_info.le_next->size = 0;
			LIST_REMOVE(&MemoryList, ptrToRemove);

			ptrToRemove = toBeFree;
			toBeFree->is_free = 0;
			toBeFree->size = 0;
			LIST_REMOVE(&MemoryList, ptrToRemove);

		}
		//case that prev is free and next not free
		else if (toBeFree->prev_next_info.le_next->is_free == 0
				&& toBeFree->prev_next_info.le_prev->is_free == 1) {
			//modifying size
			toBeFree->prev_next_info.le_prev->size += toBeFree->size;
			//modifing the pointer to the next metadata
			ptrToRemove = toBeFree;
			toBeFree->is_free = 0;
			toBeFree->size = 0;
			LIST_REMOVE(&MemoryList, ptrToRemove);

		}
		//case that next is free and prev not free
		else if (toBeFree->prev_next_info.le_next->is_free == 1
				&& toBeFree->prev_next_info.le_prev->is_free == 0) {
			//modifying size
			toBeFree->size = toBeFree->size
					+ toBeFree->prev_next_info.le_next->size;

			//modifing the pointer to the next metadata
			ptrToRemove = toBeFree->prev_next_info.le_next;
			toBeFree->prev_next_info.le_next->is_free = 0;
			toBeFree->prev_next_info.le_next->size = 0;
			LIST_REMOVE(&MemoryList, ptrToRemove);
			toBeFree->is_free = 1;
		}
		//condition that both next and prev not free

		else if (toBeFree->prev_next_info.le_next->is_free == 0
				&& toBeFree->prev_next_info.le_prev->is_free == 0) {
			toBeFree->is_free = 1;
		}
	}
//case that next is pointing to null
	else if (toBeFree->prev_next_info.le_prev != NULL
			&& toBeFree->prev_next_info.le_prev->is_free == 1) {
		//modifying size
		toBeFree->prev_next_info.le_prev->size =
				toBeFree->prev_next_info.le_prev->size + toBeFree->size;
		//modifing the pointer to the next metadata
		ptrToRemove = toBeFree;
		toBeFree->is_free = 0;
		toBeFree->size = 0;
		LIST_REMOVE(&MemoryList, ptrToRemove);
	}
	//case that next is pointing to null and prev not free
	else if (toBeFree->prev_next_info.le_prev != NULL
			&& toBeFree->prev_next_info.le_prev->is_free == 0) {
		toBeFree->is_free = 1;
	}
//case that prev is pointing to null
	else if (toBeFree->prev_next_info.le_next != NULL
			&& toBeFree->prev_next_info.le_next->is_free == 1) {
		toBeFree->size = toBeFree->size
				+ toBeFree->prev_next_info.le_next->size;
		toBeFree->is_free = 1;
		ptrToRemove = toBeFree->prev_next_info.le_next;
		toBeFree->prev_next_info.le_next->is_free = 0;
		toBeFree->prev_next_info.le_next->size = 0;
		LIST_REMOVE(&MemoryList, ptrToRemove);
	}
//case that prev is pointing to null and not free
	else if (toBeFree->prev_next_info.le_next != NULL
			&& toBeFree->prev_next_info.le_next->is_free == 0) {
		toBeFree->is_free = 1;
	}

	else
		toBeFree->is_free = 1;

}

//=========================================
// [4] REALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *realloc_block_FF(void* va, uint32 new_size) {
//TODO: [PROJECT'23.MS1 - #8] [3] DYNAMIC ALLOCATOR - realloc_block_FF()
	//panic("realloc_block_FF is not implemented yet");
	struct BlockMetaData* reAlloc = ((struct BlockMetaData *) va - 1);
	uint32 fullSize = new_size + sizeOfMetaData();
	struct BlockMetaData * splitted;
	//lazm nsheel da w n7oto f kol mkan bnt2kd en el next msh b null
	uint32 combinedSize;

	uint32 remainingSize;
	//special case 1 : size=0 & va=null
	if (new_size == 0 && va != NULL) {
		free_block(va);
		return NULL;
	}

	//general case : va and size = values
	if (va != NULL && new_size != 0) {
		//increase
		if (fullSize > reAlloc->size) {
			//case next not = null
			if (reAlloc->prev_next_info.le_next != NULL) {

				combinedSize = reAlloc->size
						+ reAlloc->prev_next_info.le_next->size;
				remainingSize = combinedSize - fullSize;
				//case next is free
				if (reAlloc->prev_next_info.le_next->is_free == 1) {
					//not enough for combining
					if (fullSize > combinedSize) {
						free_block(va);
						void* ans = alloc_block_FF(new_size);
						return ans;
					}
					//enough for combining
					else if (fullSize < combinedSize) {
						//case of remaining greater than or equal one meta data
						if (remainingSize >= sizeOfMetaData()) {
							//splitting (created new meta)
							struct BlockMetaData * splitted =
									(struct BlockMetaData *) ((uint32) reAlloc
											+ fullSize);

							splitted->size = remainingSize;
							splitted->is_free = 1;
							reAlloc->size = fullSize;
							//merging
							reAlloc->prev_next_info.le_next->size = 0;
							reAlloc->prev_next_info.le_next->is_free = 0;
							return (void*) ((void*) reAlloc + sizeOfMetaData());
						}
						//case of remaining less than one meta data
						else {
							//splitting (created new meta)
							splitted =
									(struct BlockMetaData *) ((uint32) reAlloc
											+ fullSize);
							splitted->size = remainingSize;
							splitted->is_free = 1;
							reAlloc->size = fullSize;

							return (void*) ((void*) reAlloc + sizeOfMetaData());

						}
					}
					//case combined = to fullSize
					else {
						//merge and no splitting
						reAlloc->size = combinedSize;
						//modify next meta data
						reAlloc->prev_next_info.le_next->size = 0;
						reAlloc->prev_next_info.le_next->is_free = 0;

						//mtnseesh t removy mn el list

						return (void*) ((void*) reAlloc + sizeOfMetaData());
					}
				}
				//case next is not free
				else {
					void* ans = alloc_block_FF(new_size);
					free_block(va);
					return ans;
				}	//else of case free or not
			}
			//case next = null
			else {
				void* ans = alloc_block_FF(new_size);
				free_block(va);
				return ans;
			}	//else of case next null or not
		}

		//decrease
		else if (fullSize < reAlloc->size) {
			//remaining can not fit metadata
			remainingSize = reAlloc->size - fullSize;
			if (remainingSize < sizeOfMetaData()) {
				//merge and no splitting
				reAlloc->size = fullSize;
				return (void*) ((void*) reAlloc + sizeOfMetaData());
			}
			//remaining can fit metadata
			else if (remainingSize >= sizeOfMetaData()) {
				//splitting (created new meta) and if the next is free will merge
				splitted =
						(struct BlockMetaData *) ((uint32) reAlloc + fullSize);
				splitted->size = remainingSize;
				splitted->is_free = 1;
//				if(splitted->prev_next_info.le_next->is_free==1)
				LIST_INSERT_AFTER(&MemoryList, reAlloc, splitted);
				free_block(
						(struct BlockMetaData *) ((uint32) splitted
								+ sizeOfMetaData()));
				reAlloc->size = fullSize;

				return (void*) ((void*) reAlloc + sizeOfMetaData());
			}
		}

		//same size
		else {
			return (void*) ((void*) reAlloc + sizeOfMetaData());
		}
	}	//last of general case : va and size = values

	else {
		//special case 2: va=null & size =0
		if (new_size == 0) {
			return NULL;
		}
		//special case 3: va=null & size = value
		else {
			void* ans = alloc_block_FF(new_size);
			return ans;
		}

	}

	return NULL;
}

