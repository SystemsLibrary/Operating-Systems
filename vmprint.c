#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

ssize_t readn(int fd, void *buf, size_t count)
{
	char *cbuf = buf;
	ssize_t nr, n = 0;

	while (n < count) {
		nr = read(fd, &cbuf[n], count - n);
		if (nr == 0) {
			//EOF
			break;
		} else if (nr == -1) {
			if (errno == -EINTR) {
				//retry
				continue;
			} else {
				//error
				return -1;
			}
		}
		n += nr;
	}

	return n;
}

uint64_t virt_to_phys(int fd, int pid, uint64_t virtaddr)
{
	uint64_t tbloff, tblen, pageaddr, physaddr;

	uint64_t tbl_present;
	uint64_t tbl_swapped;
	uint64_t tbl_shared;
	uint64_t tbl_pte_dirty;
	uint64_t tbl_swap_offset;
	uint64_t tbl_swap_type;

	int pagesize = (int)sysconf(_SC_PAGESIZE);

	tbloff = virtaddr / pagesize * sizeof(uint64_t);

	off_t offset = lseek(fd, tbloff, SEEK_SET);
	if ((offset == (off_t)-1) || (offset != tbloff))
	{
		printf("Open error!");
		return -1;
	}

	ssize_t nr = readn(fd, &tblen, sizeof(uint64_t));
	if (nr == -1 || nr < sizeof(uint64_t))
	{
		printf("Open error!");
		return -1;
	}
	
	tbl_present   = (tblen >> 63) & 0x1;
	tbl_swapped   = (tblen >> 62) & 0x1;
	tbl_shared    = (tblen >> 61) & 0x1;
	tbl_pte_dirty = (tblen >> 55) & 0x1;
	if (!tbl_swapped) {
		tbl_swap_offset = (tblen >> 0) & 0x7fffffffffffffULL;
	} else {
		tbl_swap_offset = (tblen >> 5) & 0x3ffffffffffffULL;
		tbl_swap_type = (tblen >> 0) & 0x1f;
	}

	pageaddr = tbl_swap_offset * pagesize;
	physaddr = (uint64_t)pageaddr | (virtaddr & (pagesize - 1));

	if (tbl_present) {
		return physaddr;
	} else {
		return -2;
	}
}

int main(int argc, char *argv[])
{
	char *arg_pid, *arg_addr, *arg_area;
	int pid;
	//long long int virtaddr, areasize;
	int pagesize;
	uint64_t virtaddr, physaddr, areasize, v;
	char procname[1024] = "";
	int fd = -1;	

	arg_pid = argv[1];
	arg_addr = argv[2];
	arg_area = argv[3];
	
	pid = (int)strtoll(arg_pid, NULL, 0);
	virtaddr = strtoll(arg_addr, NULL, 0);
	areasize = strtoll(arg_area, NULL, 0);
	printf("pid:%d, virt:0x%08llx\n", pid, virtaddr);

	memset(procname, 0, sizeof(procname));
	snprintf(procname, sizeof(procname) - 1,
		"/proc/%d/pagemap", pid);
	fd = open(procname, O_RDONLY);
	if (fd == -1)
	{
		printf("Open error!");
		return 0;
	}


	pagesize = (int)sysconf(_SC_PAGESIZE);
	virtaddr &= ~(pagesize - 1);
	
	
	int value;
	for (v = virtaddr; v < virtaddr + areasize; v += pagesize) {
		physaddr = virt_to_phys(fd, pid, v);

		if (physaddr == -1) {
			printf(" virt:0x%08llx, (%s)\n",
				(long long)v, "not valid virtual address");
			break;
		} else if (physaddr == -2) {
			printf(" virt:0x%08llx, phys:(%s)\n",
				(long long)v, "not present");
		} else {
			printf(" virt:0x%08llx, phys:0x%08llx\n",
				(long long)v, (long long)physaddr);
		}
	}
	

	return 0;
}
