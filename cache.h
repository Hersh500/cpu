#ifndef incl_cache_h
#define incl_cache_h

#include "my_types.h"

extern int cache_add_data (va_t va, uint32 tag, uint8 *data, int len);
extern int cache_get_data (va_t va, uint32 tag, uint8 *buffer, int len);
extern int cache_line_read_from_mem (va_t v_addr, pa_t p_addr);
extern int cache_line_write_to_mem (uint8 *cache_line, pa_t p_addr);
extern void invalidate ();
extern int flush();
extern void init_cache ();

#endif
