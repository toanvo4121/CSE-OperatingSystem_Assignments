#include "mem.h"
#include "stdlib.h"
#include "string.h"
#include <pthread.h>
#include <stdio.h>

static BYTE _ram[RAM_SIZE];

static struct
{
	uint32_t proc; // ID of process currently uses this page
	int index;	   // Index of the page in the list of pages allocated
				   // to the process.
	int next;	   // The next page in the list. -1 if it is the last
				   // page.
} _mem_stat[NUM_PAGES];

static pthread_mutex_t mem_lock;

void init_mem(void)
{
	memset(_mem_stat, 0, sizeof(*_mem_stat) * NUM_PAGES);
	memset(_ram, 0, sizeof(BYTE) * RAM_SIZE);
	pthread_mutex_init(&mem_lock, NULL);
}

/* get offset of the virtual address */
static addr_t get_offset(addr_t addr)
{
	return addr & ~((~0U) << OFFSET_LEN);
}

/* get the first layer index */
static addr_t get_first_lv(addr_t addr)
{
	return addr >> (OFFSET_LEN + PAGE_LEN);
}

/* get the second layer index */
static addr_t get_second_lv(addr_t addr)
{
	return (addr >> OFFSET_LEN) - (get_first_lv(addr) << PAGE_LEN);
}

static struct page_table_t *get_page_table(addr_t index, struct seg_table_t *seg_table)
{

	if (seg_table == NULL)
		return NULL;
	for (int i = 0; i < seg_table->size; i++)
	{
		if (seg_table->table[i].v_index == index)
		{
			return seg_table->table[i].pages;
		}
	}
	return NULL;
}

static int translate(addr_t virtual_addr, addr_t *physical_addr, struct pcb_t *proc)
{
	/* Offset of the virtual address */
	addr_t offset = get_offset(virtual_addr);
	/* The first layer index, find segment virtual */
	addr_t first_lv = get_first_lv(virtual_addr);
	/* The second layer index, find page virtual */
	addr_t second_lv = get_second_lv(virtual_addr);

	/* Search in the first level */
	struct page_table_t *page_table = get_page_table(first_lv, proc->seg_table);
	if (page_table == NULL)
		return 0;

	for (int i = 0; i < page_table->size; i++)
	{
		if (page_table->table[i].v_index == second_lv)
		{
			*physical_addr = (page_table->table[i].p_index << OFFSET_LEN) | (offset);
			return 1;
		}
	}
	return 0;
}

void allocate(int ret_mem, int num_pages, struct pcb_t *proc)
{

	int count_pages = 0;
	int last_index = -1; // use for update [next] in mem_stat

	for (int i = 0; i < NUM_PAGES; i++)
	{
		if (_mem_stat[i].proc != 0)
			continue;					  // page in used
		_mem_stat[i].proc = proc->pid;	  // page is used by proc
		_mem_stat[i].index = count_pages; // index in list of allocated pages

		if (last_index > -1)
			_mem_stat[last_index].next = i; // update next
		last_index = i;						// update last page

		addr_t virtual_addr = ret_mem + count_pages * PAGE_SIZE; // virtual address of this page
		addr_t seg = get_first_lv(virtual_addr);

		struct page_table_t *page_table = get_page_table(seg, proc->seg_table);

		if (page_table == NULL)
		{
			proc->seg_table->table[proc->seg_table->size].v_index = seg;
			proc->seg_table->table[proc->seg_table->size].pages = (struct page_table_t *)malloc(sizeof(struct page_table_t));
			page_table = proc->seg_table->table[proc->seg_table->size].pages;
			proc->seg_table->size++;
		}

		page_table->table[page_table->size].v_index = get_second_lv(virtual_addr);
		page_table->table[page_table->size].p_index = i; // format of i is 10 bit segment and page in address
		page_table->size++;

		count_pages++;
		if (count_pages == num_pages)
		{
			_mem_stat[i].next = -1; // last page in list
			break;
		}
	}
}

addr_t alloc_mem(uint32_t size, struct pcb_t *proc)
{
	pthread_mutex_lock(&mem_lock);
	addr_t ret_mem = 0;

	// Number of pages we will use for this process
	uint32_t num_pages = size / PAGE_SIZE;
	if (size % PAGE_SIZE)
		num_pages++;

	// memory available? We could allocate new memory region or not?
	int mem_avail = 0;
	int free_pages = 0;
	for (int i = 0; i < NUM_PAGES; i++)
	{
		if (_mem_stat[i].proc == 0)
			free_pages++;
	}
	if (free_pages >= num_pages && proc->bp + num_pages * PAGE_SIZE < RAM_SIZE)
		mem_avail = 1;

	if (mem_avail)
	{
		ret_mem = proc->bp;
		proc->bp += num_pages * PAGE_SIZE;
		allocate(ret_mem, num_pages, proc);
	}
	pthread_mutex_unlock(&mem_lock);
	return ret_mem;
}

int free_mem(addr_t address, struct pcb_t *proc)
{

	pthread_mutex_lock(&mem_lock);

	addr_t virtual_addr = address; // virtual address to free
	addr_t physical_addr = 0;	   // physical address to free

	// Have physical page in memory ?
	if (translate(virtual_addr, &physical_addr, proc) == 0)
	{
		pthread_mutex_unlock(&mem_lock);
		return 1;
	}
	// Clear physical page in memory
	int num_pages = 0; // number of pages
	int i = 0;
	for (i = physical_addr >> OFFSET_LEN; i != -1; i = _mem_stat[i].next)
	{
		num_pages++;
		_mem_stat[i].proc = 0; // clear physical memory
	}

	// Clear virtual page in process
	for (i = 0; i < num_pages; i++)
	{
		addr_t seg = get_first_lv(virtual_addr + i * PAGE_SIZE);
		addr_t page = get_second_lv(virtual_addr + i * PAGE_SIZE);
		// virtual_addr + i * PAGE_SIZE = address of this page

		struct page_table_t *page_table = get_page_table(seg, proc->seg_table);
		if (page_table == NULL)
			continue;

		//remove page
		for (int j = 0; j < page_table->size; j++)
		{
			if (page_table->table[j].v_index == page)
			{
				page_table->size--;
				page_table->table[j] = page_table->table[page_table->size];
				break;
			}
		}
		//remove page table
		if (page_table->size == 0 && proc->seg_table != NULL)
		{
			for (int a = 0; a < proc->seg_table->size; a++)
			{
				if (proc->seg_table->table[a].v_index == seg)
				{
					proc->seg_table->size--;
					proc->seg_table->table[a] = proc->seg_table->table[proc->seg_table->size];
					proc->seg_table->table[proc->seg_table->size].v_index = 0;
					free(proc->seg_table->table[proc->seg_table->size].pages);
				}
			}
		}
	}
	proc->bp -= num_pages * PAGE_SIZE;
	pthread_mutex_unlock(&mem_lock);
	return 0;
}

int read_mem(addr_t address, struct pcb_t *proc, BYTE *data)
{
	addr_t physical_addr;
	if (translate(address, &physical_addr, proc))
	{
		*data = _ram[physical_addr];
		return 0;
	}
	else
	{
		return 1;
	}
}

int write_mem(addr_t address, struct pcb_t *proc, BYTE data)
{
	addr_t physical_addr;
	if (translate(address, &physical_addr, proc))
	{
		_ram[physical_addr] = data;
		return 0;
	}
	else
	{
		return 1;
	}
}

void dump(void)
{
	int i;
	for (i = 0; i < NUM_PAGES; i++)
	{
		if (_mem_stat[i].proc != 0)
		{
			printf("%03d: ", i);
			printf("%05x-%05x - PID: %02d (idx %03d, nxt: %03d)\n",
				   i << OFFSET_LEN,
				   ((i + 1) << OFFSET_LEN) - 1,
				   _mem_stat[i].proc,
				   _mem_stat[i].index,
				   _mem_stat[i].next);
			int j;
			for (j = i << OFFSET_LEN;
				 j < ((i + 1) << OFFSET_LEN) - 1;
				 j++)
			{

				if (_ram[j] != 0)
				{
					printf("\t%05x: %02x\n", j, _ram[j]);
				}
			}
		}
	}
}
