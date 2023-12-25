/*
 * fault_handler.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include "trap.h"
#include <kern/proc/user_environment.h>
#include "../cpu/sched.h"
#include "../disk/pagefile_manager.h"
#include "../mem/memory_manager.h"

//2014 Test Free(): Set it to bypass the PAGE FAULT on an instruction with this length and continue executing the next one
// 0 means don't bypass the PAGE FAULT
uint8 bypassInstrLength = 0;

//===============================
// REPLACEMENT STRATEGIES
//===============================
//2020
void setPageReplacmentAlgorithmLRU(int LRU_TYPE) {
	assert(
			LRU_TYPE == PG_REP_LRU_TIME_APPROX || LRU_TYPE == PG_REP_LRU_LISTS_APPROX);
	_PageRepAlgoType = LRU_TYPE;
}
void setPageReplacmentAlgorithmCLOCK() {
	_PageRepAlgoType = PG_REP_CLOCK;
}
void setPageReplacmentAlgorithmFIFO() {
	_PageRepAlgoType = PG_REP_FIFO;
}
void setPageReplacmentAlgorithmModifiedCLOCK() {
	_PageRepAlgoType = PG_REP_MODIFIEDCLOCK;
}
/*2018*/void setPageReplacmentAlgorithmDynamicLocal() {
	_PageRepAlgoType = PG_REP_DYNAMIC_LOCAL;
}
/*2021*/void setPageReplacmentAlgorithmNchanceCLOCK(int PageWSMaxSweeps) {
	_PageRepAlgoType = PG_REP_NchanceCLOCK;
	page_WS_max_sweeps = PageWSMaxSweeps;
}

//2020
uint32 isPageReplacmentAlgorithmLRU(int LRU_TYPE) {
	return _PageRepAlgoType == LRU_TYPE ? 1 : 0;
}
uint32 isPageReplacmentAlgorithmCLOCK() {
	if (_PageRepAlgoType == PG_REP_CLOCK)
		return 1;
	return 0;
}
uint32 isPageReplacmentAlgorithmFIFO() {
	if (_PageRepAlgoType == PG_REP_FIFO)
		return 1;
	return 0;
}
uint32 isPageReplacmentAlgorithmModifiedCLOCK() {
	if (_PageRepAlgoType == PG_REP_MODIFIEDCLOCK)
		return 1;
	return 0;
}
/*2018*/uint32 isPageReplacmentAlgorithmDynamicLocal() {
	if (_PageRepAlgoType == PG_REP_DYNAMIC_LOCAL)
		return 1;
	return 0;
}
/*2021*/uint32 isPageReplacmentAlgorithmNchanceCLOCK() {
	if (_PageRepAlgoType == PG_REP_NchanceCLOCK)
		return 1;
	return 0;
}

//===============================
// PAGE BUFFERING
//===============================
void enableModifiedBuffer(uint32 enableIt) {
	_EnableModifiedBuffer = enableIt;
}
uint8 isModifiedBufferEnabled() {
	return _EnableModifiedBuffer;
}

void enableBuffering(uint32 enableIt) {
	_EnableBuffering = enableIt;
}
uint8 isBufferingEnabled() {
	return _EnableBuffering;
}

void setModifiedBufferLength(uint32 length) {
	_ModifiedBufferLength = length;
}
uint32 getModifiedBufferLength() {
	return _ModifiedBufferLength;
}

//===============================
// FAULT HANDLERS
//===============================

//Handle the table fault
void table_fault_handler(struct Env * curenv, uint32 fault_va) {
	//panic("table_fault_handler() is not implemented yet...!!");
	//Check if it's a stack page
	uint32* ptr_table;
#if USE_KHEAP
	{
		ptr_table = create_page_table(curenv->env_page_directory,
				(uint32) fault_va);
	}
#else
	{
		__static_cpt(curenv->env_page_directory, (uint32)fault_va, &ptr_table);
	}
#endif
}

//Handle the page fault

void page_fault_handler(struct Env * curenv, uint32 fault_va) {
#if USE_KHEAP
	struct WorkingSetElement *victimWSElement = NULL;
	uint32 wsSize = LIST_SIZE(&(curenv->page_WS_list));
#else
	int iWS = curenv->page_last_WS_index;
	uint32 wsSize = env_page_ws_get_size(curenv);
#endif

	if (isPageReplacmentAlgorithmFIFO()) {
			struct WorkingSetElement * wsElem = NULL;
			if (wsSize < (curenv->page_WS_max_size)) {
				//cprintf("PLACEMENT=========================WS Size = %d\n", wsSize );
				//TODO: [PROJECT'23.MS2 - #15] [3] PAGE FAULT HANDLER - Placement
				// Write your code here, remove the panic and write your code
				fault_va = ROUNDDOWN(fault_va, PAGE_SIZE);
				struct FrameInfo* frame;
				allocate_frame(&frame);
				map_frame(curenv->env_page_directory, frame, fault_va,
					PERM_USER | PERM_WRITEABLE);
				frame->va = fault_va;
				int ret = pf_read_env_page(curenv, (void*) fault_va);
				if (ret == E_PAGE_NOT_EXIST_IN_PF) {
					//if in user heap/stack
					if (((fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX)
							|| (fault_va >= USTACKBOTTOM && fault_va < USTACKTOP))) {
					} else {
						sched_kill_env(curenv->env_id);
					}
				}
				wsElem = env_page_ws_list_create_element(curenv, fault_va);
				LIST_INSERT_TAIL(&(curenv->page_WS_list), wsElem);

				if (LIST_SIZE(&curenv->page_WS_list)
						== (curenv->page_WS_max_size)) {
					curenv->page_last_WS_element = LIST_FIRST(
							&(curenv->page_WS_list));
					curenv->page_last_WS_index=0;
				} else {
					curenv->page_last_WS_element = NULL;
				}

			} else {

				//TODO: [PROJECT'23.MS3 - #1] [1] PAGE FAULT HANDLER - FIFO Replacement
				// Write your code here, remove the panic and write your code
				struct FrameInfo* ptr_frame_info = NULL;
				//allocate in RAM;
				allocate_frame(&ptr_frame_info);
				map_frame(curenv->env_page_directory, ptr_frame_info, fault_va,
				PERM_USER | PERM_WRITEABLE);
				ptr_frame_info->va = fault_va;
				//read from file and check if exist or not
				int ret = pf_read_env_page(curenv, (void*) fault_va);
				if (ret == E_PAGE_NOT_EXIST_IN_PF) {
					//if in user heap/stack
					if (((fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX)
							|| (fault_va >= USTACKBOTTOM && fault_va < USTACKTOP))) {

					} else {
						sched_kill_env(curenv->env_id);
					}
				}
				uint32 victim = curenv->page_last_WS_element->virtual_address;

				//if modiified write it on disk
				int found = pt_get_page_permissions(curenv->env_page_directory,
						victim);
				if ((found & PERM_MODIFIED) == PERM_MODIFIED) {

					uint32 * ptr_page_table = NULL;
					struct FrameInfo * f_info = get_frame_info(
							curenv->env_page_directory, victim, &ptr_page_table);
					pf_update_env_page(curenv, victim, f_info);
				}
				//delete victim from RAM
				unmap_frame(curenv->env_page_directory, victim);
				//law wselt l 2kher el list
				if (curenv->page_last_WS_element->prev_next_info.le_next == NULL) {
					env_page_ws_invalidate(curenv, victim);
					wsElem = env_page_ws_list_create_element(curenv, fault_va);
					LIST_INSERT_TAIL(&(curenv->page_WS_list), wsElem);
					curenv->page_last_WS_element = LIST_FIRST(
							&(curenv->page_WS_list));
				}
				//gher keda
				else {
					env_page_ws_invalidate(curenv, victim);
					wsElem = env_page_ws_list_create_element(curenv, fault_va);
					LIST_INSERT_BEFORE(&(curenv->page_WS_list),
							curenv->page_last_WS_element, wsElem);
				}
//				curenv->page_last_WS_index=(curenv->page_last_WS_index+1)%curenv->page_WS_max_size;
				//panic("page_fault_handler() FIFO Replacement is not implemented yet...!!");

			}
		}
	if (isPageReplacmentAlgorithmLRU(PG_REP_LRU_LISTS_APPROX)) {
		//TODO: [PROJECT'23.MS3 - #2] [1] PAGE FAULT HANDLER - LRU Replacement
		// Write your code here, remove the panic and write your code
		//panic("page_fault_handler() LRU Replacement is not implemented yet...!!");
		int insec = 0;
		uint32 ActiveSize = LIST_SIZE(&(curenv->ActiveList));
		uint32 SecondSize = LIST_SIZE(&(curenv->SecondList));

		if ((ActiveSize + SecondSize) < curenv->page_WS_max_size) {
			/* faulted when address not in mem, or in mem but present=0*/
			fault_va = ROUNDDOWN(fault_va, PAGE_SIZE);
			struct WorkingSetElement * active_tail = LIST_LAST(
					&(curenv->ActiveList));
			//Scenario 1: Placement
			//TODO: [PROJECT'23.MS3 - #2] [1] PAGE FAULT HANDLER – LRU Placement

			struct WorkingSetElement * tmp;
			LIST_FOREACH(tmp, &(curenv->SecondList))
			{
				if (ROUNDDOWN(tmp->virtual_address,PAGE_SIZE) == fault_va) {
					insec = 1;
					break;
				}

			}
			if ((LIST_SIZE(&(curenv->ActiveList)) == curenv->ActiveListSize)
					&& insec
					&& (LIST_SIZE(&(curenv->SecondList))
							< curenv->SecondListSize)) {
				LIST_REMOVE(&(curenv->SecondList), tmp);
				//head of sc = tail of active
				pt_set_page_permissions(curenv->env_page_directory,
						active_tail->virtual_address, 0,
						PERM_USER | PERM_WRITEABLE | PERM_PRESENT);
				LIST_REMOVE(&(curenv->ActiveList), active_tail);
				LIST_INSERT_HEAD(&(curenv->SecondList), active_tail);
				//remove tail of active list
				//tmp3 = curenv->ActiveList.lh_last;
				//insert tmp into active list
				pt_set_page_permissions(curenv->env_page_directory,
						tmp->virtual_address,
						PERM_PRESENT | PERM_WRITEABLE | PERM_USER, 0);
				LIST_INSERT_HEAD(&(curenv->ActiveList), tmp);
				return;

			}
			//if not in SC list
			struct FrameInfo* frame;
			allocate_frame(&frame);

			map_frame(curenv->env_page_directory, frame, fault_va,
			PERM_USER | PERM_WRITEABLE);
			frame->va = fault_va;

			//is it found in PF or not?

			int ret = pf_read_env_page(curenv, (void*) fault_va);
			if (ret == E_PAGE_NOT_EXIST_IN_PF) {
				//if in user heap/stack
				if (((fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX)
						|| (fault_va >= USTACKBOTTOM && fault_va < USTACKTOP))) {

				} else {
					sched_kill_env(curenv->env_id);
				}
			}
			struct WorkingSetElement * wsElem = env_page_ws_list_create_element(
					curenv, fault_va);

			//if active list not full:
			if (LIST_SIZE(&(curenv->ActiveList)) < curenv->ActiveListSize) {
				pt_set_page_permissions(curenv->env_page_directory, fault_va,
						PERM_PRESENT, 0);
				//& add in active list
				LIST_INSERT_HEAD(&(curenv->ActiveList), wsElem);
			} else {
				pt_set_page_permissions(curenv->env_page_directory, fault_va,
				PERM_PRESENT, 0);
				LIST_REMOVE(&(curenv->ActiveList), active_tail);

				pt_set_page_permissions(curenv->env_page_directory,
						active_tail->virtual_address, 0, PERM_PRESENT);
				LIST_INSERT_HEAD(&(curenv->SecondList), active_tail);
				//remove tail of active list
				//insert_head fault_va in active head
				LIST_INSERT_HEAD(&curenv->ActiveList, wsElem);
				env_page_ws_print(curenv);
			}

		}
			else {
			//Scenario 2: Replacement
			//TODO: [PROJECT'23.MS3 - #1] [1] PAGE FAULT HANDLER - LRU Replacement
			//panic("lRU replacement not implemented yet ");
			int insec = 0;

			/* faulted when address not in mem, or in mem but present=0*/
			fault_va = ROUNDDOWN(fault_va, PAGE_SIZE);
			struct WorkingSetElement * Second_tail = LIST_LAST(
					&(curenv->SecondList));

			struct WorkingSetElement * active_tail = LIST_LAST(
					&(curenv->ActiveList));
			struct WorkingSetElement * tmp;
			LIST_FOREACH(tmp, &(curenv->SecondList))
			{
				if (ROUNDDOWN(tmp->virtual_address,PAGE_SIZE) == fault_va) {
					insec = 1;
					break;
				}

			}
			if (insec) {
				LIST_REMOVE(&(curenv->SecondList), tmp);
				//head of sc = tail of active
				pt_set_page_permissions(curenv->env_page_directory,
						active_tail->virtual_address, 0,
						PERM_USER | PERM_WRITEABLE | PERM_PRESENT);
				LIST_REMOVE(&(curenv->ActiveList), active_tail);
				LIST_INSERT_HEAD(&(curenv->SecondList), active_tail);

				pt_set_page_permissions(curenv->env_page_directory,
						tmp->virtual_address,
						PERM_PRESENT | PERM_WRITEABLE | PERM_USER, 0);
				LIST_INSERT_HEAD(&(curenv->ActiveList), tmp);
				return;

			}

			//if not in SC list
			struct FrameInfo* frame;
			allocate_frame(&frame);

			map_frame(curenv->env_page_directory, frame, fault_va,
			PERM_USER | PERM_WRITEABLE);
			frame->va = fault_va;

			//is it found in PF or not?

			int ret = pf_read_env_page(curenv, (void*) fault_va);
			if (ret == E_PAGE_NOT_EXIST_IN_PF) {
				//if in user heap/stack
				if (((fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX)
						|| (fault_va >= USTACKBOTTOM && fault_va < USTACKTOP))) {

				} else {
					sched_kill_env(curenv->env_id);
				}
			}
			struct WorkingSetElement * wsElem = env_page_ws_list_create_element(
					curenv, fault_va);
			//if active list full:
			uint32 victim = curenv->SecondList.lh_last->virtual_address;
			//if modiified write it on disk
			int found = pt_get_page_permissions(curenv->env_page_directory,
					victim);
			if ((found & PERM_MODIFIED) == PERM_MODIFIED) {

				uint32 * ptr_page_table = NULL;
				struct FrameInfo * f_info = get_frame_info(
						curenv->env_page_directory, victim, &ptr_page_table);
				pf_update_env_page(curenv, victim, f_info);
			}
			//delete victim from RAM
			unmap_frame(curenv->env_page_directory, victim);
			env_page_ws_invalidate(curenv, victim);
			wsElem = env_page_ws_list_create_element(curenv, fault_va);

			pt_set_page_permissions(curenv->env_page_directory, fault_va,
			PERM_PRESENT, 0);
			LIST_REMOVE(&(curenv->ActiveList), active_tail);

			pt_set_page_permissions(curenv->env_page_directory,
					active_tail->virtual_address, 0, PERM_PRESENT);
			LIST_INSERT_HEAD(&(curenv->SecondList), active_tail);
			//remove tail of active list
			//insert_head fault_va in active head
			LIST_INSERT_HEAD(&curenv->ActiveList, wsElem);

			//}
				}
		}
}

void __page_fault_handler_with_buffering(struct Env * curenv, uint32 fault_va) {
	panic("this function is not required...!!");
}
