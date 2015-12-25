#include <stdio.h>
#include <stdlib.h>

#include "my_types.h" 
#include "bitset.h"
#include "vm.h"



static pa_t start_phys_address;
static uint32 num_pages;
static uint8 *user_page_bitset;

int lookup_page_table (va_t v_addr, pgd_t *page_table, pa_t *p_addr) 
{
	pmd_t *pmd;	
	pte_t pte;
	
	pmd = page_table->pmd[v_addr >> 24];
	if (!pmd) {
		return -1;
	}
	pte = pmd->pte[(v_addr & 0x00FFF000) >> 12]; 	
	if ((pte.flags | PAGE_VALID) != PAGE_VALID) {
		return -1;
	}
	*p_addr = pte.physical_addr + (v_addr & 0x00000FFF);
	return 0;
}

int create_free_pool (pa_t start_addr, uint32 number_of_pages)
{
	user_page_bitset = create_bitset (number_of_pages);		
	if (!user_page_bitset) {
		return -1;
	}
	num_pages = number_of_pages;
	start_phys_address = start_addr;
	return 0;
}

static uint32 allocate_page ()
{
	int i;
	
	for (i = 0; i < num_pages; i++) {
		if (is_set (i, user_page_bitset) == 0) {
			set_bit (i, user_page_bitset);
			return (start_phys_address + (i<<12));
		}
	}
	return -1;
}

static int free_page (pa_t page_addr)
{	
	uint32 bit;		
	
	if ((page_addr & (PAGE_SIZE - 1)) != 0) {
		//printf("Page addr = %8x\n", page_addr);
		printf ("ERR: NOt aligned on page boundary\n");
		return -1;
	}	

	bit = (page_addr - start_phys_address) >> 12;
	clr_bit (bit, user_page_bitset);
	return 0;
}

static pgd_t *create_page_table ()
{
	int i;
	pgd_t *page_table;

	page_table = malloc (sizeof (pgd_t));
	if (!page_table) {
		return NULL;
	}
	for (i = 0; i < FIRST_LEVEL_SIZE; i++) {
		page_table->pmd[i] = NULL;
	}
	return page_table;	
}

static int allocate_memory (va_t start_addr, va_t end_addr, pgd_t *page_table) 
{
	uint32 tmp_addr;	
	pmd_t *tmp_pmd;

	if ((start_addr & 4095) != 0) {
		printf ("Start Addr not aligned on page boundary\n");
		return -1;
	}
	/*	
	if ((end_addr & 4095) != 0) {
		printf ("End Addr not aligned on page boundary\n");
		return -1;
	}
	*/
	tmp_addr = start_addr;

	while (tmp_addr <= end_addr) {
		if (!page_table->pmd[tmp_addr >> 24]) {
			tmp_pmd = malloc (sizeof (pmd_t));	
			if (!tmp_pmd) {
				printf("Unable to allocate memory for pmd\n");
				return -1;
			}
			page_table->pmd[tmp_addr >> 24] = tmp_pmd;
		}

		page_table->pmd[tmp_addr >> 24]->pte[(tmp_addr & 0x00FFF000) >> 12].physical_addr = allocate_page ();
		page_table->pmd[tmp_addr >> 24]->pte[(tmp_addr & 0x00FFF000) >> 12].flags |= PAGE_VALID;
		/*TODO:Do something with flags */
		tmp_addr = tmp_addr + 4096;
	}
	return 0;
}


mm_t *create_addr_space (va_t code_start, uint32 code_size, va_t data_start, uint32 data_size, va_t stack_start, uint32 stack_size) 
{
	mm_t *mm;
	int rc;
	pgd_t *page_table;
	
	mm = malloc (sizeof (mm_t));
	if (!mm) {
		return NULL;
	}

	page_table = create_page_table ();
	if (!page_table) {
		printf ("malloc failed for page table\n");
		return NULL;
	}

	mm->pgd = page_table;
	mm->code_start = code_start;
	mm->code_end = code_start + code_size;
	rc = allocate_memory (mm->code_start, mm->code_end, mm->pgd);	
	if (rc < 0) {
		return NULL;
	}

	//printf ("allocated code\n");

	if (data_size != 0) {
		mm->data_start = data_start;
		mm->data_end = data_start + data_size;
		rc = allocate_memory (mm->data_start, mm->data_end, mm->pgd);
		if (rc < 0) {
			return NULL;
		}
	}

	//printf ("### allocated data\n");

	if (stack_size != 0) {
		mm->stack_start = stack_start;
		mm->stack_end = stack_start + stack_size;
		rc = allocate_memory (mm->stack_start, mm->stack_end, mm->pgd);
		if (rc < 0) {
			return NULL;
		}
	}
	
	//printf ("### allocated stack\n");
	mm->v_mem_space = NULL;

	return mm;
}


static void free_memory (va_t start, va_t end, pgd_t *page_table) 
{
	va_t tmp_addr;
	uint32 prev;
	tmp_addr = start;
	pmd_t *page_md;
	
	tmp_addr = start;
	prev = tmp_addr >> 24;

	while (tmp_addr <= end) {
		page_md = page_table->pmd[prev];
		if (!page_md) {
			printf ("ERROR: tmp_addr 0x%x page_md not set at index 0x%x\n", tmp_addr, prev);
			tmp_addr += 4096;
			continue;
		}
		if ((page_md->pte[(tmp_addr & 0x00FFF000) >> 12].flags & PAGE_VALID) != PAGE_VALID) {
			tmp_addr += 4096;
			continue;
		}
		free_page (page_md->pte[(tmp_addr & 0x00FFF000) >> 12].physical_addr);

		if (tmp_addr >> 24 != prev) {
			free (page_md);
			prev = tmp_addr >> 24;
		}
		tmp_addr += 4096;
	}
}

void free_addr_space (mm_t *mm) 
{
	pgd_t *page_table;
	
	page_table = mm->pgd;
	free_memory (mm->code_start, mm->code_end, page_table);
	free_memory (mm->data_start, mm->data_end, page_table);
	free_memory (mm->stack_start,mm->stack_end, page_table);
	free (mm->pgd);
	free (mm);
}

#if DEBUG
int main ()
{
	mm_t *mm;	
	
	create_free_pool (0, 4096);
	mm = create_addr_space (0xA0001000, 0xA0010000, 4096, 8192, 12288, 16384);
	free_addr_space (mm);
}
#endif
