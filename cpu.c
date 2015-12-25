#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>

#include "my_types.h"
#include "tlb.h"
#include "vm.h"
#include "cache.h"
#include "mem_api.h"

extern int errno;

#define R0  cpu_registers[0]
#define R1  cpu_registers[1]
#define R2  cpu_registers[2]
#define R3  cpu_registers[3]
#define R4  cpu_registers[4]
#define R5  cpu_registers[5]
#define R6  cpu_registers[6]
#define R7  cpu_registers[7]
#define R8  cpu_registers[8]
#define R9  cpu_registers[9]
#define R10 cpu_registers[10]
#define R11 cpu_registers[11]
#define R12 cpu_registers[12]
#define R13 cpu_registers[13]
#define R14 cpu_registers[14]
#define PC  cpu_registers[15]


enum {NOP, LDIMM, LD, SD, JMP, PRINTC, PRINTI, BNE, BEQ, BGT, BLT, BGE, BLE, ADD, SUB};
char *instructions[] = {"NOP", "LDIMM", "LD", "SD", "JMP", "PRINTC", "PRINTI", 
                        "BNE", "BEQ", "BGT", "BLT", "BGE", "BLE", "ADD", "SUB"}; 

static uint32 cpu_registers [16];
static task_t *current;


uint32 program1 [] = {
			0x01030000,
			0x01020001,
			0x01000041,
			0x0101001a,
			0x05000000,
			0x0d002000,
			0x0e112000,
			0x07103010,
			0x00000000,
			0x00000000,
			0x00000000,
			0x00000000,
};


typedef struct task_queue {
	task_t task;
	struct task_queue *next;
} task_queue_t;

void save_ctx (task_t *task)
{
	memcpy (task->ctx.cpu_registers, cpu_registers, sizeof (cpu_registers));
}

void restore_ctx (task_t *task)
{
	memcpy (cpu_registers, task->ctx.cpu_registers, sizeof (cpu_registers));
}


int load (va_t v_addr, pgd_t *page_table, uint32 *dst)
{
	pa_t p_addr;
	uint8 perms;
	int rc;

	rc = translate (v_addr, &p_addr, &perms);	
	if (rc < 0) {
		//printf ("translate failed for v_addr %x\n", v_addr);
		rc = lookup_page_table (v_addr, page_table, &p_addr); /*TODO*/
		if (rc < 0) {
			printf("Segfault\n");
			return -1;
		}
		//printf ("adding tlb entry for v_addr %x p_addr %x\n", 
		//	v_addr, p_addr);
		add_tlb_entry (v_addr, p_addr, 0);
	}

	//printf ("getting data from cache v_addr %x\n", v_addr);
	rc = cache_get_data (v_addr, (p_addr & 0xFFFFF000), (uint8 *)dst, 4);
	if (rc < 0) {
		//printf ("reading cache line from memory v_addr %x\n", v_addr);
		cache_line_read_from_mem (v_addr, p_addr);
		rc = cache_get_data (v_addr, (p_addr & 0xFFFFF000), (uint8 *)dst, 4);
		if (rc < 0) {
			printf ("ERROR: something is wrong at line num %d!\n", __LINE__);
			return -1;
		}
	}
	return 0;
}

/*TODO: NEED TO DO SOMETHING WITH DIRTY BIT */
int store (va_t v_addr, pgd_t *page_table, uint32 *data)
{
	pa_t *p_addr;
	uint8 perms;
	int rc;
	
	p_addr = malloc (sizeof (pa_t));	
	rc = translate (v_addr, p_addr, &perms);
	if (rc < 0) {
		rc = lookup_page_table (v_addr, page_table, p_addr);
		if (rc < 0) {
			printf("Segfault\n");
			return -1;
		}
		add_tlb_entry (v_addr, *p_addr, 0);
	}
	rc = cache_add_data (v_addr, (*p_addr & 0xFFFFF000), (uint8 *)data, 4);
	return rc;
}

static int 
decode_instr (uint32 instr, uint32 *opcode, uint32 *sreg, uint32 *dreg, 
              uint32 *treg, uint32 *val)
{
	*opcode = (instr & 0xFF000000) >> 24;
	if (*opcode > SUB) {
		printf("Invalid instruction\n");
		return -1;
	}
	//printf("Instruction = %s\n", instructions[*opcode]);
	*sreg = (instr & 0x00F00000) >> 20;
	*dreg = (instr & 0x000F0000) >> 16;
	*treg = (instr & 0x0000F000) >> 12;
	*val = instr & 0x00000FFF;
	return 0;
}

static uint32 fetch_instruction (uint32 pc) 
{	
	uint32 instr;	

	if (load (pc, current->mem_descriptor->pgd, &instr) < 0) {
		printf("unable to get instruction\n");
		return 0;
	}
	return instr;
}

static void execute_instruction (uint32 instr, uint32 *pc)
{
	uint32 opcode, sreg, dreg, treg, imm_val;
	uint32 vaddr;

	decode_instr (instr, &opcode, &sreg, &dreg, &treg, &imm_val);

	switch (opcode) {
		case LDIMM:
			cpu_registers [dreg] = imm_val;	
			*pc = *pc + 4;
			break;

		case LD:			
			vaddr = sreg + imm_val;
			if (load (vaddr, current->mem_descriptor->pgd, &cpu_registers[dreg]) < 0) {	
				printf ("segmentation violation? %x\n", vaddr);
				exit (0);
			}
			*pc = *pc + 4;
			break;

		case SD:
			vaddr = dreg + imm_val;
			if (store (vaddr, current->mem_descriptor->pgd, &cpu_registers[sreg]) < 0) {
				printf ("segmentation violation? %x\n", vaddr);
				exit (0);
			}
			*pc = *pc + 4;
			break;

		case JMP:
			*pc = imm_val;
			break;

		case ADD:
			cpu_registers[dreg] = cpu_registers[sreg] + cpu_registers[treg];
			*pc = *pc + 4;
			break;

		case SUB:
			cpu_registers[dreg] = cpu_registers[sreg] - cpu_registers[treg];
			*pc = *pc + 4;
			break;

		case PRINTC:
			printf ("%c", cpu_registers[sreg]);
			*pc = *pc + 4;
			break;

		case PRINTI:
			printf ("%u", cpu_registers[sreg]);
			*pc = *pc + 4;
			break;

		case BEQ:
			if (cpu_registers[sreg] == cpu_registers[treg]) {
				*pc = imm_val; 
			}
			else {
				*pc = *pc + 4;
			}
			break;

		case BNE:
			if (cpu_registers[sreg] != cpu_registers[treg]) {
				*pc = imm_val; 
			}
			else {
				*pc = *pc + 4;
			}
			break;

		case BGE:
			if (cpu_registers[sreg] >= cpu_registers[treg]) {
				*pc = imm_val; 
			}
			else {
				*pc = *pc + 4;
			}
			break;

		case BLE:
			if (cpu_registers[sreg] <= cpu_registers[treg]) {
				*pc = imm_val; 
			}
			else {
				*pc = *pc + 4;
			}
			break;

		case BLT:
			if (cpu_registers[sreg] < cpu_registers[treg]) {
				*pc = imm_val; 
			}
			else {
				*pc = *pc + 4;
			}
			break;

		case BGT:
			if (cpu_registers[sreg] > cpu_registers[treg]) {
				*pc = imm_val; 
			}
			else {
				*pc = *pc + 4;
			}
			break;

		case NOP:
			*pc = *pc + 4;
			break;

		default:
			printf("TRAP: invalid instruction \n");
			exit(-1);
	}
}



int main (int argc, char *argv[])
{
	mm_t *mem_desc1, *mem_desc2;
	task_t *task1, *task2;
	uint32 instr, i;
	pa_t paddr;


	if (argc != 2) {
		printf ("usage: cpu <ip addr of memory sim daemon >\n");
		exit (1);
	}

	init_mem_lib (argv[1]);
	init_tlb ();
	init_cache ();

	if (create_free_pool (0, 4096) < 0) {
		printf ("Unable to allocate bitset\n");
		exit (-1);
	}

	//printf ("creating addr space...\n");
	mem_desc1 = create_addr_space (0, 64, 0, 0, 0, 0);
	if (!mem_desc1) {
		printf ("ERROR: failed to create mem space\n");
		exit (-2);
	}

	task1 = malloc (sizeof (task_t));
	if (!task1) {
		printf ("ERROR: failed to create task\n");
		exit (-2);
	}

	task1->mem_descriptor = mem_desc1;
	task1->ctx.cpu_registers[15] = 0;

	current = task1;

	/* KLUGE ALERT */
	for (i = 0; i < sizeof(program1)/sizeof(uint32); i++) {
		//printf ("### lookup addr %x\n", i*4);
		if (lookup_page_table (i<<2, current->mem_descriptor->pgd, &paddr) < 0) {
			printf("ERROR:unable to map va to pa in lookup\n");
			exit (-3);
		}
		//printf ("writing %x at addr %x\n", program1[i], paddr);
		if (mem_write_32 (paddr, program1[i]) < 0) {
			printf("ERROR: could not write to main mem\n");
			exit (-4);
		}
	} 


	PC = task1->ctx.cpu_registers[15];

	while (1) {
		instr = fetch_instruction (PC);
		//printf ("fetched instr %x\n", instr);
		if (instr == 0) {
			exit (0);
		}
		execute_instruction (instr, &PC);
	}
	return 0;
}
