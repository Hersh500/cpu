#ifndef _incl_mem_api_h
#define _incl_mem_api_h

#include "my_types.h"

extern int mem_read_32 (pa_t phys_addr, uint32 *data);
extern int mem_write_32 (pa_t phys_addr, uint32 data);
extern int init_mem_lib (char *ip_addr);


#endif
