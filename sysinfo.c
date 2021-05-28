#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

void main(int argc, char **argv) {
	int pid;
	sscanf(argv[1], "%d", &pid);
	
	
	char filename[1000];
    	sprintf(filename, "/proc/%d/stat", pid);
    	FILE *f = fopen(filename, "r");
	
	if (f == NULL)
	{
    		perror("Error");
		return;
	}
	
	int process;
    	char comm[100];
    	char state;
    	int ppid;
	int pgrd;
	fscanf(f, "%d %s %c %d %d", &process, comm, &state, &ppid, &pgrd);
	printf("process = %d\n", process);
	printf("comm = %s\n", comm);
    	printf("state = %c\n", state);
    	printf("parent pid = %d\n", ppid);
	printf("process group id = %d\n\n", pgrd);
    	fclose(f);
	
	FILE *g;

	if ((g = fopen("/proc/meminfo", "r")) == 0)
	{
    		fprintf(stderr, "Failed to open %s for reading (%d: %s)\n", "/proc/meminfo", errno, strerror(errno));
		exit(1);
	}

	unsigned long memtotal;
	unsigned long memfree;
	unsigned long memavailable;
	char line[256];
    	while(fgets(line, sizeof(line), g))
    	{
        	int ram;
        	if(sscanf(line, "MemTotal: %d kB", &ram) == 1)
        	{
			printf("Total Memory: %d\n", ram);
            		fclose(g);
            		return;
        	}
    }
	
}
