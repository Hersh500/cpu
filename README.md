#A simulator for a simple RISC CPU in C 

Before discussing the cpu module, we’ll first discuss a couple of the critical components. 

##Cache 

###Background: 
The cache is one of the fundamental building blocks of the modern CPU. To speed up performance, the processor will typically load information that is accessed from main memory into the cache. 

When the processor wants to get some data from main memory, it checks first whether or not that data is in the cache (if it is found in the cache, it’s called a cache hit). 
If it isn’t (called a cache miss), it will read from main memory, and load a certain block of fixed size around that data into the cache . This block is called a cache line. This is based on the idea of spatial locality, or the idea that if the data at a certain memory location is requested, data at surrounding memory locations will also be requested soon. 

Also copied into the cache is the physical memory address of the data (so the processor can find the data in the cache). This is called the tag. There is also the index, which is calculated from the virtual address. This is used to address into the cache itself. 

There are three different ways to implement a cache. Two of them are direct-hit and fully associative. In direct hit, for each unique index, there is only one location in the cache. In a fully associative cache, any index can be put in any location in the cache. 
Each of these has their own benefits and drawbacks. In direct hit, it is easy to locate data, but problems arise when writing to the cache, because there may be more conflicts (data is already in the cache, but you have to write to that location). 
In fully-associative, writing to the cache is easy, but locating a particular index is hard because the entire cache may have to be crawled.

###My implementation:
In my implementation, I combine these two into a 4-way set associative cache. 
What that essentially means is that for each tag, there are four possible sets that the data could be in. 
This is the best compromise between a direct-hit cache and fully associative. 
Each cache line is represented using a structure called cache_entry. The cache itself is a 2-dimensional array. 
The first dimension represents the number of possible indices (the index is calculated based on the virtual address), and the second dimension represents the 4-way associativity of the cache (see the cache_add_data method of cache.c). 

Here's a really concise description of all of the parts of the cache program (ping me if you have any more questions about any of these modules): 

•	The two methods cache_line_read_from_mem and cache_line_write_to_mem are helper methods that read and write from/to main memory. 

•	cache_add_data is where most of the work is done. 
- It first calculates the index and offset of the data from its virtual address. 
- It then walks through the four possible lines where the data could be (b/c of 4-way associativity), and compares the tag of each to the data it is searching for. If it is found in a valid line, then we have a cache hit, and it copies the data at that particular offset into the cache line. The dirty bit, which represents whether or not the cache and main memory are in sync, is set to 1, which means they are out of sync.
- If there is a cache miss (that particular tag was not found), then it first searches to see if any of the cache lines at that index is invalid (the valid bit is 0). If it is, then it is overwritten.
	
- If no invalid line is found, one of the old entries at that index is booted out, and the data is added to the cache. 	

- cache_get_data operates in the same way as cache_add_data, but reads instead of writes. 

- It searches at a particular index for the tag; if it is not found, then it is a cache miss, and the method returns -1. This means that, later, we will have to read from main memory. 
	
- If it is found, then the correct data is copied into the buffer that is passed in, and the method returns 0. 

•	flush is a method that syncs the cache and main memory. It will copy all changes made to data in the cache to the appropriate data in main memory. 

•	invalidate is a method meant to simulate what happens when the CPU first boots. Since the cache has to be empty on startup (but it may not be; the memory could be set to any random sequence), the processor will just invalidate all lines in the cache. 

##Virtual Memory

###Background: 
	The only memory that is actually available to the processor is the main memory, different segments of which will be used by various processes, or user programs. However, the OS wants to give the illusion to all processes that they have the entire main memory available to them. It does this by providing each process with virtual address space. 
Virtual memory is divided into contiguous segments, called pages. A structure called the page table maps virtual memory page addresses into physical addresses. When a process makes a call, say to store some data in some virtual address, the processor looks up the physical address and stores the data in the corresponding page in physical memory. 
	Each process is given its own memory space. This memory space is usually divided into a couple of different sections. One section is reserved for the code, or the actual instructions that the processor has to execute for the program. Some is for the stack, or the local variables stored in each function in the program. The rest is for the data of the program, such as global variables. 

###My implementation: 
•	The lookup_page_table() function indexes into a page table (that is passed in) and locates the corresponding pmd and pte structures. From this, it can find the corresponding physical address. If the address is not found, then it returns -1. This is called a page fault. 

•	The function create_free_pool() takes in a physical address and the number of pages to allocate, and allocates a new virtual address space. It also initializes a bitset, which is just a string of bits, used in this case to keep track of which pages are valid (have valid data stored in them). 

•	allocate_page() simulates the process in which the OS allocates memory to processes. This function simply selects the first available page and allocates it. free_page() does the opposite, with a given page address.

•	Create_page_table() serves as a helper method that initializes the page table. 

•	allocate_memory() simulates the allocation of an address space and populates the page table. It walks through, page by page, and adds entries to the page table and calls allocate_page(). It does this by creating a temporary pmd, which represents this chunk of memory (if it doesn’t exist already). 

•	create_addr_space() simulates the creation of a new process in memory. It takes in the virtual addresses of the code, data, and stack sections, as well as their sizes. It then initializes a structure that represents a process’s memory space (see vm.h -> mm_struct). It then goes through and calls allocate_memory() for the code, data, and stack sections, building the page table along the way. 

##TLB (Translation Lookaside Buffer)

###Background:
To actually obtain useful data, the processor must look up information in main memory, which is addressed using physical addresses. However, the processor assigns each process a virtual address space to use. Therefore, to load or store data into physical memory, the processor has to translate a process’s virtual address into a physical address. There are two locations in which these translations are stored. 
	
The first is the page table. This is a ‘large’ section of memory that contains the mappings of all virtual addresses into physical addresses. Because it is stored in main memory, accessing the page table is quite slow. Therefore, the processor caches certain translations in the translation lookaside buffer.  It functions similarly to the cache described above; it is a smaller piece of SRAM memory that the processor checks before translating any virtual address into a physical address. 

###My implementation: 
	I implemented the TLB as a chained linked-list hash table. 
	
•	The methods init_tlb(), size_of_hash_list(), add_to_hash_list(), and hash() are just helper methods. 

•	When the processor wants to obtain a physical address from a virtual address, it calls translate(). translate() finds the appropriate bucket in the TLB, and walks the list to find the entry with the corresponding physical address. It returns 0 on success, and -1 on failure. If this method fails, the processor will have to make a call to the page table, and then add the entry to the TLB later.

•	The method add_tlb_entry() adds an entry to the tlb. It functions similarly to translate() in that it first calculates the hash to find the appropriate bucket, and then if there are fewer than 8 entries in the linked-list (an arbitrary length), then it will simply add the entry to its head. If there are 8 entries, then it replaces the first entry (again, an arbitrary decision) with this translation. 

•	Similar to the cache, we need a way to invalidate entries at startup; naïve_invalidate() does this. 

##Physical Memory

###Background
	There isn’t much to explain for the background of physical memory. It is divided up into pages, much like virtual memory. However, this is memory that is “shared” amongst the OS and all the other processes running on the computer. 

###My implementation
	
	There are two pertinent programs here: mem_api.c and phys_mem.c
•	My implementation uses sockets to simulate the memory bus. This way, the processor program can make requests to the physical memory program, which is running simultaneously.

•	The structure mem_op represents a memory operation. It contains the address, data, and the request. The memory itself is represented as an array of bytes. 

•	The main loop of phys_mem listens to the socket, and when it receives a request, it writes it into a mem_op structure. Then, based on the value of cmd, the program either writes to or reads from the main memory array. It then sends a response back to the processor. 

•	mem_api acts as the memory bus, providing simple methods for cpu.c to interact with phys_mem. 

•	mem_read_32 takes in a physical address and a data buffer, and sends a request over the bound socket to the physical memory. If the request is successful, it copies the data into the buffer. 

•	mem_write_32 functions in a similar manner; it takes in a physical address and some data, and sends a write request to the physical memory. If either method gets a failure, this program will throw a bus error. 

##CPU

###Background

This is where everything gets put together. The CPU itself is actually quite simple. It simply follows a constant loop of fetching instructions and executing them. 
The CPU typically has a set of instructions that it can execute, called an instruction set. For this project, I created my own instruction set, which follows a RISC paradigm. Typically, most instructions are either 32-bit of 64-bit quantities. MIPS has three different types of instructions: R, I, and J. Each starts, with a 6-bit opcode, and the differences in each refers to the number of registers the instruction operates on. R type instructions operate on 3 registers; I on 2, and J just on an address. 

The very basic pipeline for what goes on in the CPU is this; it will first fetch the next instruction from the address stored in the program counter. 
It will then decode this instruction (get the opcode, registers, and immediate values if there are any), and then execute the instruction. 
What happens at this step depends on the instruction. If it is an instruction where something needs to be computed (ie. add, sub, bne), it will send the appropriate data to the ALU. 
If it is a jump, it will simply modify the address at the program counter. 

When executing processes, a CPU typically also has to do context switches, where it will save the register values and other data for one process, and then switch to executing another process. 
It does many of these switches per second, to give the illusion that it is executing all of these processes simultaneously. 

###My implementation 
•	On “startup”, my simulation resets the cache, tlb, loads a process into main memory, and loads the address for that process (which I’ve hard coded into the program) into the program counter. It then goes into a loop of fetching and executing instructions (I have the functions for context switching written, but haven’t implemented them into the CPU functionality)

•	My instruction set is structured as such: the first 8 bits are reserved for the opcode, or the actual command. Then, there are 4 bits for the src register, dst register, and target register each. The final 12 bits are for any immediate values (take a look at program1 and see if you can tell what it does). The possible opcodes themselves are listed in the “instructions” array. 

•	The function load() is called for the load instruction. It first checks the tlb to translate the virtual address into a physical address. If the entry is not in the tlb, it looks in the page table, and adds the entry to the tlb. It then checks the cache for the data, and if the data is not in the cache, it loads the data into the cache from physical memory, and then reads from the cache again. 

•	The function store() operates similarly. It will try to translate, and then calls cache_add_data() to store the data (when the cache is flushed, that data will be written back to physical memory). 

•	decode_instr() is what determines the opcode, registers, and value from the instruction itself. It accomplishes this by masking out the irrelevant parts of the instruction. 

•	fetch_instruction() takes the address in the pc, and calls load() to load the instruction from the cache or main memory. 

•	execute_instr() is where the brunt of the work is done. It first calls decode_instr() to determine the registers, opcode, and value. Then, using a switch/case statement on the opcode, it makes the appropriate calls to execute the instruction. For example, for a load immediate instruction, it will write the immediate value to the specified register. For jump, it will modify the pc by the appropriate amount.  

###A Minor Note

My simulation (well, it walks the line between a simulation and emulation) actually incorporates a little bit the work the Operating System does as well. So it's not strictly all "CPU". For example, the allocation of memory for processes (the free pool, bitset, virtual memory, etc.) aren't really CPU-level tasks (for a better idea for what goes on in the CPU itself, check out my verilog-cpu repo). These are things the OS takes care of on a typical computer, so technically I've also written parts of an OS into this emulation/simulation. 
