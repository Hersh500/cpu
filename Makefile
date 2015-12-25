CC = gcc
CFLAGS = -g -I. -Wall -Wfatal-errors
DEPS = tlb.h cache.h vm.h bitset.h my_types.h  mem_api.h
CPU_OBJ = tlb.o cache.o vm.o bitset.o cpu.o mem_api.o
PHYS_MEM_OBJ = phys_mem.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

cpu: $(CPU_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) 
phys_mem: $(PHYS_MEM_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) 

.PHONY: all clean

all: cpu phys_mem

clean:
	rm -f *.o
	rm -f cpu
	rm -f phys_mem
