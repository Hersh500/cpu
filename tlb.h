void init_tlb ();
int translate (uint32 va, uint32 *pa, uint8 *perms);
int add_tlb_entry (uint32 va, uint32 pa, uint8 perms);
