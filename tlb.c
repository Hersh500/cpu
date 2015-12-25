#include <stdio.h>
#include <stdlib.h>
#include "my_types.h"

#define MAX_HASH_LENGTH 16


typedef struct tlb_entry_s {
	uint32 va;
	uint32 pa;
	uint8 perms;
	struct tlb_entry_s *next;
	struct tlb_entry_s *global_next;
} tlb_entry;


static tlb_entry *tlb [MAX_HASH_LENGTH];

void init_tlb () 
{
	int i;
	for (i = 0; i < MAX_HASH_LENGTH; i++) {
		tlb [i] = NULL;
	}
}

int size_of_hash_list (int h) 
{
	tlb_entry *tmp = tlb[h];
	int count = 0;
	while (tmp != NULL) {
		tmp = tmp->next;
		count++;
	}	
	return (count);
}

void add_to_hash_list (tlb_entry **list, tlb_entry *entry) 
{
	entry->next = *list;
	*list = entry;
}


static uint32 hash (uint32 va) 
{
	uint32 v = va & 0xFFFFF000;
	/* uint32 v = va & ~0x3FF; */
	v = v >> 12;
	return (v % MAX_HASH_LENGTH);
}


int translate (uint32 va, uint32 *pa, uint8 *perms)
{
	uint32 h;		
	tlb_entry *tmp;

	h = hash (va);
	tmp = tlb [h];
	while (tmp != NULL) {
		if (tmp->va == (va & 0xFFFFF000)) {
			*pa = tmp->pa + (va & 0x00000FFF);
			*perms = tmp->perms;
			return 0;
		}	
		tmp = tmp->next;
	}
	return -1;
}

void naive_invalidate ()
{
	int i;
	tlb_entry *t_next;
	tlb_entry *tmp;


	for (i = 0; i < MAX_HASH_LENGTH; i++) {
		tmp = tlb [i];	
		while (tmp != NULL) {
			t_next = tmp->next;	
			free (tmp);
			tmp = t_next;
		}
		tlb [i] = NULL;
	}

	return;
}

int add_tlb_entry (uint32 va, uint32 pa, uint8 perms)
{
	uint32 h;
	tlb_entry *t;

	h = hash (va);
	
	if (size_of_hash_list (h) < 8) {
		t = malloc (sizeof (tlb_entry));
		if (t == NULL) {
			return (-1);
		}
		t->va = va & 0xFFFFF000;
		t->pa = pa;
		t->perms = perms;
		add_to_hash_list (&tlb[h], t);
		return 0;
	}
	else {
		/*Right now, just replacing the first one*/
		/*TODO:decide which entry to replace*/
		tlb_entry *entry_to_modify = tlb[h];
		entry_to_modify->va = va;
		entry_to_modify->pa = pa;
		entry_to_modify->perms = perms;
		return 0;
	}
}

void print_tlb ()
{
	int i; 
	tlb_entry *tmp;

	for (i = 0; i < MAX_HASH_LENGTH; i++) {
		tmp = tlb[i];		
		printf("Hash bucket %d\n", i);
		while (tmp != NULL) {
			printf("VA %u; PA %u; PERMS %u\n", tmp->va, tmp->pa, tmp->perms);
			tmp = tmp->next;
		}
	}
}
#if DEBUG
int main()
{
	return 0;
}
#endif
