#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

typedef unsigned char uint8;
typedef unsigned int uint32;
typedef unsigned long uint64;

#define PHYS_MEM_START 0x00000000

static uint8 *memory;
extern int errno;

enum {MEM_RD_REQ, MEM_WR_REQ, MEM_RD_RESP, MEM_WR_RESP};

typedef struct mem_op {
	uint32 addr;
	uint32 data;
	uint8 cmd;
	uint8 status;
} mem_op_t;


int main (int argc, char *argv[])
{
	int sd, n;
	uint32 mem_sz;
	struct sockaddr_in servaddr, clientaddr;
	mem_op_t mesg, resp;
	socklen_t addrlen;
	uint32 offset;

	if (argc < 2) {
		printf ("usage: mem_sim_d <memory size in bytes>\n");
		exit (1);
	}

	mem_sz = atoi (argv[1]);

	memory = malloc (mem_sz);
	if (!memory) {
		printf ("malloc failed sz = %u\n", mem_sz);
		exit (2);
	}

	sd = socket (AF_INET, SOCK_DGRAM, 0);
	if (sd < 0) {
		printf ("socket failed..errno = %d\n", errno);
		exit (3);
	}

	memset (&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons (32001);

	if (bind (sd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		printf ("bind failed..errno = %d\n", errno);
		exit (4);
	}
	
	while (1) {
		n = recvfrom (sd, (void *) &mesg, sizeof(mesg), 0, (struct sockaddr *)&clientaddr, &addrlen);
		if (n == sizeof(mesg)) {
			if (mesg.addr < PHYS_MEM_START || mesg.addr > (PHYS_MEM_START + mem_sz - 4)) {
				printf ("invalid mesg.addr = %u %u\n", mesg.addr, PHYS_MEM_START + mem_sz - 4);
				resp.status = -1;
			}
			else {
				resp.status = 0;
			}

			offset = mesg.addr - PHYS_MEM_START;
			switch (mesg.cmd) {
				case MEM_RD_REQ:
					resp.cmd =  MEM_RD_RESP;
					memcpy (&resp.data, &memory[offset], 4); /**/
					printf ("READ at address %u : %x\n", offset, 
						                     *((uint32 *)(memory+offset))); 
					break;

				case MEM_WR_REQ:
					memcpy (&memory[offset], &mesg.data, 4); /**/
					resp.cmd =  MEM_WR_RESP;
					printf ("WRITE at address %u : %x %x\n", offset, 
						                     *((uint32 *)(memory+offset)), mesg.data); 
					break;

				default:
					break;	
			}
			sendto (sd, &resp, sizeof(resp), 0, (struct sockaddr *)&clientaddr, addrlen);
		} 
	}
	return 0;
}
