#include <stdio.h>
#include <stdlib.h>
#include "my_types.h"

uint8 *create_bitset (uint32 number_of_bits)
{
	int bytes_to_alloc; 
	uint8 *array_of_bits;

	bytes_to_alloc = (number_of_bits >> 3) + 1;	
	array_of_bits = malloc (bytes_to_alloc);
	if (array_of_bits == NULL) {
		printf("Malloc failed. \n");
		return NULL;
	}
	else {
		return array_of_bits;
	}
}

static inline int trickery (int bit)
{
	return (1 << (8 - ((bit + 1) % 8)));
}

int is_set (int bit, uint8 *array_of_bits)
{
	int index;	
	uint8 num;

	index = bit >> 3;
	num = array_of_bits[index];	
	
	//Have to do this trickery because we're doing LSB first
	if ((num & trickery (bit)) == trickery (bit)) {
		return 1;
	}
	return 0;
}

void set_bit (int bit, uint8 *array_of_bits) 
{	
	int index;
	uint8 num;
	
	index = bit / 8;
	num = array_of_bits[index];	
	array_of_bits[index] = num | trickery (bit);	
}

void clr_bit (int bit, uint8 *array_of_bits) 
{
	int index;
	uint8 num;
	
	index = bit >> 3;
	num = array_of_bits[index];	
	array_of_bits[index] = num & ~(trickery (bit));	
}

/*
static void print_bitset ()
{
	int i;

	for (i = 0; i < bytes; i++) {	
		printf ("%u ", array_of_bits[i]);
	}
	printf("\n");
}

int main ()
{
	create_bitset (36);
	set_bit (1);
	set_bit (35);
	print_bitset();
	
	clr_bit (1);
	clr_bit (35);
	print_bitset ();
	set_bit (0);
	print_bitset ();
	return 0;
}
*/
