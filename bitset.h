#ifndef include_snd_buffer_h
#define include_snd_buffer_h

#include "my_types.h"

extern uint8 *create_bitset (uint32 number_of_bits);
extern int is_set (int bit, uint8 *array_of_bits);
extern void set_bit (int bit, uint8 *array_of_bits);
extern void clr_bit (int bit, uint8 *array_of_bits);

#endif
