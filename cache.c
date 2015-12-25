#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "my_types.h"
#include "mem_api.h"
#include "cache.h"

#define VALID_BIT 0x01
#define DIRTY_BIT 0x02 
#define NUM_SET 4
#define NUM_INDICES 16 
#define BLOCK_SIZE 256


typedef struct {
	uint32 tag; /*Max Value is 2^20-1 */ 
	uint8 flags;
	uint8 data [BLOCK_SIZE];
} cache_entry;

cache_entry *cache [NUM_INDICES] [NUM_SET];

long cache_hits = 0;
long cache_misses = 0;

void init_cache ()
{
	int i, j;
	for (i = 0; i < NUM_INDICES; i++) {
		for (j = 0; j < NUM_SET; j++) {
			cache[i][j] = NULL;
		}
	}	
}

int cache_line_write_to_mem (uint8 *data, pa_t p_addr) 
{
	int i, rc;

	for (i = 0; i < 64; i++) {
		rc = mem_write_32 (p_addr, ((uint32 *)data)[i]);
		if (rc < 0) {	
			printf ("ERROR: mem_write_32 failed for p_addr %x\n", p_addr);
			return -1;
		}
	}
	return 0;
}

int cache_line_read_from_mem (va_t v_addr, pa_t p_addr) 
{
	uint32 buffer[64]; 
	int i, rc;

	for (i = 0; i < 64; i++) {
		//printf ("reading mem at addr %u\n", p_addr + (i * 4));
		rc = mem_read_32 (p_addr + (i * 4), &buffer[i]);
		if (rc < 0) {	
			printf ("ERROR: mem_read_32 failed for p_addr %x\n", p_addr);
			return -1;
		}
		//printf ("read %u : %x\n", i, buffer[i]);
	}
	
	rc = cache_add_data (v_addr, (p_addr & 0xFFFFF000), (uint8 *)buffer, 256);
	return rc;
}

//4-way set associative cache

int cache_add_data (uint32 va, uint32 tag, uint8 *data, int len)
{
	int i;
	uint32 index;
	uint32 offset;
	cache_entry *t;
	
	//printf ("cache_add_data va = %x tag = %x\n", va, tag);
	for (i = 0; i < len; i++) {
		//printf ("adding cache data [%u] = %x\n", i, data[i]);
	}

	index = (va & 0x00000F00) >> 8;
	offset = va & 0x000000FF;
	
	cache_entry **c = cache [index];
	
	for (i = 0;	i < NUM_SET; i++) {
		if (c[i] && (c[i]->tag == tag && (c[i]->flags & VALID_BIT))) {
			//printf ("found va %x in cache\n", va);
			cache_hits++;
			//printf ("updating index %u offset %u\n", index, offset);
			memcpy ((uint8 *)(c[i]->data) + offset, data, len);
			c[i]->flags |= DIRTY_BIT;
			return 0;
		}
	}
	
	printf ("did not find va/tag in cache!\n");

	for (i = 0;	i < NUM_SET; i++) {
		if (!c[i] || ((c[i]->flags & VALID_BIT) != VALID_BIT)) {
			t = malloc (sizeof (cache_entry));
			if (!t) {
				return -1;
			}
			t->tag = tag;
			memcpy ((uint8 *)(t->data) + offset, data, len);
			t->flags = t->flags | VALID_BIT;
			if (c[i]) {
				free (c[i]);
			}
			c[i] = t;
			c[i]->flags |= DIRTY_BIT;
			cache_misses++;
			return 0;
		}
	}

	// IF ALL BLOCKS ARE VALID BUT NONE OF THE TAGS MATCH, NEED TO KICK ONE OUT
	t = malloc (sizeof (cache_entry));
	if (!t) {
		return -1;
	}
	t->tag = tag;
	memcpy ((t->data) + offset, data, len);
	t->flags = t->flags | VALID_BIT;
	free (c[0]);
	c[0] = t;
	c[0]->flags |= DIRTY_BIT;
	cache_misses++;
	return 0;
}


int cache_get_data (uint32 va, uint32 tag, uint8 *buffer, int len)
{
	int i;
	uint32 index = (va & 0x00000F00) >> 8;
	uint32 offset = va & 0x000000FF;
	
	//printf ("cache get data va %x tag %x\n", va, tag);

	cache_entry **c = cache[index]; /*Why is this pointer to pointer */
	
	for (i = 0; i < NUM_SET; i++) {
		if (c[i] != NULL && ((c[i]->flags & VALID_BIT) == VALID_BIT) && c[i]->tag == tag) {
				memcpy (buffer, &(c[i]->data[offset]), len);
				//printf ("found data in cache index %u offset %u\n", index, offset);
				cache_hits++;
				return 0;
		}
	}
	cache_misses++;
	return -1;
}

/*writes the contents of the cache to physical memory*/
int flush ()
{
	int i,j;	
	pa_t phys_addr;
	
	for (i = 0; i < NUM_INDICES; i++) {
		for (j = 0; j < NUM_SET; j++) {
			if ((cache [i][j]->flags & DIRTY_BIT) == DIRTY_BIT) {
				phys_addr = cache[i][j]->tag + (i << 8); 
				if (cache_line_write_to_mem (cache[i][j]->data, phys_addr))  {
					printf ("ERROR: cache_line_write_mem failed\n");
					return -1;
				}
				cache[i][j]->flags &= ~DIRTY_BIT;
			}
		}
	}
	return 0;
}

/*Sets valid bit to 0*/
void invalidate ()
{
	int i;
	int j;
	
	for (i = 0; i < NUM_INDICES; i++) {
		for (j = 0; j < NUM_SET; j++) {
			cache[i][j]->flags &= ~VALID_BIT;
		}
	}
}
#if DEBUG
int main() 
{
	return 0;
}
#endif
