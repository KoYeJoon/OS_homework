/**********************************************************************
 * Copyright (c) 2020
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "types.h"
#include "list_head.h"
#include "vm.h"

/**
 * Ready queue of the system
 */
extern struct list_head processes;

/**
 * Currently running process
 */
extern struct process *current;

/**
 * Page Table Base Register that MMU will walk through for address translation
 */
extern struct pagetable *ptbr;


/**
 * The number of mappings for each page frame. Can be used to determine how
 * many processes are using the page frames.
 */
extern unsigned int mapcounts[];


/**
 * alloc_page(@vpn, @rw)
 *
 * DESCRIPTION
 *   Allocate a page frame that is not allocated to any process, and map it
 *   to @vpn. When the system has multiple free pages, this function should
 *   allocate the page frame with the **smallest pfn**.
 *   You may construct the page table of the @current process. When the page
 *   is allocated with RW_WRITE flag, the page may be later accessed for writes.
 *   However, the pages populated with RW_READ only should not be accessed with
 *   RW_WRITE accesses.
 *
 * RETURN
 *   Return allocated page frame number.
 *   Return -1 if all page frames are allocated.
 */
unsigned int alloc_page(unsigned int vpn, unsigned int rw)
{
//	printf("I'm alloc\n");
	int out = vpn/16;
	int in = vpn%16;
	if(current->pagetable.outer_ptes[out]==NULL)
	{
		current->pagetable.outer_ptes[out]=malloc(sizeof(struct pte_directory));
//		printf("hi\n");

	}
	if(current->pagetable.outer_ptes[out]->ptes[in].valid==false)
	{
		int pfn =0;
		for(int i=0;i<NR_PAGEFRAMES;i++)
		{
			if(mapcounts[i]==0)
			{
				pfn = i;
				break;
			}
		}
		mapcounts[pfn]++;
		current->pagetable.outer_ptes[out]->ptes[in].pfn = pfn;
		current->pagetable.outer_ptes[out]->ptes[in].valid = true;
		//printf("%d\n",rw);
		if(rw == RW_READ)
		{
			current->pagetable.outer_ptes[out]->ptes[in].writable = 0;
		}
		else
		{
//			printf("hi\n");
			current->pagetable.outer_ptes[out]->ptes[in].writable = RW_WRITE;
		}
		return pfn;
	}
	return -1;
}


/**
 * free_page(@vpn)
 *
 * DESCRIPTION
 *   Deallocate the page from the current processor. Make sure that the fields
 *   for the corresponding PTE (valid, writable, pfn) is set @false or 0.
 *   Also, consider carefully for the case when a page is shared by two processes,
 *   and one process is to free the page.
 */
void free_page(unsigned int vpn)
{
//	printf("I'm free\n");
	int out = vpn/16;
	int in = vpn%16;
	int pfn = current->pagetable.outer_ptes[out]->ptes[in].pfn;

	current->pagetable.outer_ptes[out]->ptes[in].valid = false;
	current->pagetable.outer_ptes[out]->ptes[in].writable = 0;
	current->pagetable.outer_ptes[out]->ptes[in].private = 0;
	if(mapcounts[pfn]<2)
	{
		current->pagetable.outer_ptes[out]->ptes[in].pfn = 0;
	}
	mapcounts[pfn]--;
}


/**
 * handle_page_fault()
 *
 * DESCRIPTION
 *   Handle the page fault for accessing @vpn for @rw. This function is called
 *   by the framework when the __translate() for @vpn fails. This implies;
 *   0. page directory is invalid
 *   1. pte is invalid
 *   2. pte is not writable but @rw is for write
 *   This function should identify the situation, and do the copy-on-write if
 *   necessary.
 *
 * RETURN
 *   @true on successful fault handling
 *   @false otherwise
 */
bool handle_page_fault(unsigned int vpn, unsigned int rw)
{
	int out = vpn/16;
	int in = vpn%16;
//	printf("I'm handling\n");
	//page directory invalid
	int pfn = 0;
	for(int i = 0;i<NR_PAGEFRAMES;i++)
	{
		pfn = pfn+mapcounts[i];
	}
	if(current->pagetable.outer_ptes[out]==NULL)
	{
//		printf("I'm handling1\n");
		current->pagetable.outer_ptes[out]=malloc(sizeof(struct pte_directory));
		current->pagetable.outer_ptes[out]->ptes[in].pfn = alloc_page(vpn,rw);
		return true;
	}
	//pte invaild
	if(current->pagetable.outer_ptes[out]->ptes[in].valid == false)
	{
//		printf("I'm handling2\n");
		current->pagetable.outer_ptes[out]->ptes[in].pfn = alloc_page(vpn,rw);
		
		return true;
	}
	//pte writeable X rw write
	if(current->pagetable.outer_ptes[out]->ptes[in].writable==0 && rw==RW_WRITE && current->pagetable.outer_ptes[out]->ptes[in].private != 0)
	{
//		printf("I'm handling3\n");
		if(mapcounts[current->pagetable.outer_ptes[out]->ptes[in].pfn]>=2)
		{
			mapcounts[current->pagetable.outer_ptes[out]->ptes[in].pfn]--;
			current->pagetable.outer_ptes[out]->ptes[in].valid = false;

			current->pagetable.outer_ptes[out]->ptes[in].pfn = alloc_page(vpn, RW_WRITE);
			return true;
		}
		else
		{
			current->pagetable.outer_ptes[out]->ptes[in].writable = RW_WRITE;
			return true;
		}
	}
	return false;
}


/**
 * switch_process()
 *
 * DESCRIPTION
 *   If there is a process with @pid in @processes, switch to the process.
 *   The @current process at the moment should be put into the @processes
 *   list, and @current should be replaced to the requested process.
 *   Make sure that the next process is unlinked from the @processes, and
 *   @ptbr is set properly.
 *
 *   If there is no process with @pid in the @processes list, fork a process
 *   from the @current. This implies the forked child process should have
 *   the identical page table entry 'values' to its parent's (i.e., @current)
 *   page table. 
 *   To implement the copy-on-write feature, you should manipulate the writable
 *   bit in PTE and mapcounts for shared pages. You may use pte->private for 
 *   storing some useful information :-)
 */
void switch_process(unsigned int pid)
{
//	printf("I'm switching");
	struct process*p = NULL;
	list_for_each_entry(p,&processes,list)
	{
//		printf("%d\n",p->pid);
//		printf("%d\n",pid);
		if(p->pid == pid)
		{
			list_add_tail(&current->list,&processes);
			current = p;
			ptbr = &(current->pagetable);
			list_del_init(&current->list);
			return;
		}
	}
	
	//no process -> fork
	printf("I'm switching\n");
	struct process*child = malloc(sizeof(struct process));
	child->pid = pid;
	for(int i=0;i<16;i++)
	{

		child->pagetable.outer_ptes[i]=NULL;
		if(current->pagetable.outer_ptes[i]!=NULL)
		{
			child->pagetable.outer_ptes[i]=malloc(sizeof(struct pte_directory));
			for(int j=0;j<16;j++)
			{
				if(current->pagetable.outer_ptes[i]->ptes[j].valid == true)
				{
					mapcounts[current->pagetable.outer_ptes[i]->ptes[j].pfn]++;
				}
				child->pagetable.outer_ptes[i]->ptes[j] = current->pagetable.outer_ptes[i]->ptes[j];

				if(child->pagetable.outer_ptes[i]->ptes[j].writable == RW_WRITE)
				{
					child->pagetable.outer_ptes[i]->ptes[j].private = RW_READ;
					current->pagetable.outer_ptes[i]->ptes[j].private = RW_READ;
				}
				child->pagetable.outer_ptes[i]->ptes[j].writable = 0;
				current->pagetable.outer_ptes[i]->ptes[j].writable = 0;
			}
				
		}
	}
	ptbr = &(child->pagetable);
	list_add_tail(&current->list,&processes);
	current=child;
	
}
