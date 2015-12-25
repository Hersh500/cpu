#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>

#include "my_types.h"


extern int errno;

enum {MEM_RD_REQ, MEM_WR_REQ, MEM_RD_RESP, MEM_WR_RESP};

typedef struct mem_op {
	uint32 addr;
	uint32 data;
	uint8 cmd;
	uint8 status;
} mem_op_t;


static int mem_sd;
static struct sockaddr_in servaddr;
static struct sockaddr_in tmpaddr;
static socklen_t tmpaddrlen;


void bus_error (uint32 addr)
{
	printf ("bus error at addr %u\n", addr);	
}


int mem_read_32 (pa_t phys_addr, uint32 *data)
{
	mem_op_t req, resp;
	int n;

	req.cmd = MEM_RD_REQ;	
	req.addr = phys_addr;
	
	sendto (mem_sd, &req, sizeof(req), 0, (struct sockaddr *)&servaddr, sizeof(servaddr)); 
	n = recvfrom (mem_sd, &resp, sizeof(resp), 0, (struct sockaddr *)&tmpaddr, &tmpaddrlen);
	if (n == sizeof(resp) && resp.cmd == MEM_RD_RESP) {
		if (resp.status == 0) {
			*data = resp.data;
		}
		else {
			printf ("resp.status = %d\n", resp.status);
			bus_error (phys_addr);
		}
		return 0;
	}
	return -1;
}


int mem_write_32 (pa_t phys_addr, uint32 data)
{
	mem_op_t req, resp;
	int n;

	req.cmd = MEM_WR_REQ;	
	req.addr = phys_addr;
	req.data = data;
	
	sendto (mem_sd, &req, sizeof(req), 0, (struct sockaddr *)&servaddr, sizeof(servaddr)); 
	n = recvfrom (mem_sd, &resp, sizeof(resp), 0, (struct sockaddr *)&tmpaddr, &tmpaddrlen);
	if (n == sizeof(resp) && resp.cmd == MEM_WR_RESP) {
		if (resp.status != 0) {
			printf ("resp.status = %d\n", resp.status);
			bus_error(phys_addr);
		}
		return 0;
	}
	return -1;
}


int init_mem_lib (char *ip_addr)
{
	mem_sd = socket (AF_INET, SOCK_DGRAM, 0);
	if (mem_sd < 0) {
		printf ("socket failed..errno = %d\n", errno);
		return -1;
	}

	memset (&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr (ip_addr);
	servaddr.sin_port = htons (32001);

	return 0;
}
