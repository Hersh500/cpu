#ifndef incl_vm_h
#define incl_vm_h

#include "my_types.h"

#define FIRST_LEVEL_SIZE 256
#define SECOND_LEVEL_SIZE 4096
#define PAGE_SIZE 4096
#define PROCESS_NAME_SIZE 32
#define PAGE_VALID 0x1


typedef struct vma {
	va_t start_vm_addr;
	va_t end_vm_addr;
	uint32 flags;
	struct vma *next;
} vma_t;

typedef struct pte {
	pa_t physical_addr;
	uint8 flags;
} pte_t;

typedef struct pmd {
	pte_t pte [SECOND_LEVEL_SIZE];
} pmd_t;

typedef struct page_d {
	pmd_t *pmd [FIRST_LEVEL_SIZE];
} pgd_t;

typedef struct mm_struct {
	vma_t *v_mem_space;
	va_t code_start;
	va_t code_end;
	va_t data_start;
	va_t data_end;
	va_t stack_start;
	va_t stack_end;
	pgd_t *pgd;
} mm_t;


typedef struct process_ctx {
	uint32 cpu_registers [16];
} process_ctx_t;

typedef struct task {
	uint32 pid;
	uint8 name [PROCESS_NAME_SIZE];
	uint32 running_ticks;
	uint32 state;
	mm_t *mem_descriptor;
	process_ctx_t ctx;
} task_t;


extern int lookup_page_table (va_t v_addr, pgd_t *page_table, pa_t *p_addr);
extern mm_t *create_addr_space (va_t code_start, uint32 code_size, va_t data_start, 
                             uint32 data_size, va_t stack_start, uint32 stack_size);
extern void free_addr_space (mm_t *mm);
extern int create_free_pool (pa_t start_addr, uint32 number_of_pages);

#endif
