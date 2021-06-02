/*
 * mmap.c
 *
 *  Created on: 2 Jun 2021
 *      Author: Grant
 */

/*
 * vmprint.c
 *
 *  Created on: 29 May 2021
 *      Author: Grant
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

FILE *backStore;
FILE *addressFile;

/*
#define LINELENGTH 10

#define PAGESIZE 256
int pageTable[PAGESIZE];
int pageFrame[PAGESIZE];

#define TLB_LENGTH 16
int TLBPage[TLB_LENGTH];
int TLBFrame[TLB_LENGTH];
int TLBNum = 0;
int TLBCounter = 0;

#define FRAMELENGTH 256
char readBacker[FRAMELENGTH];

#define physicalMemoryBytes 65536
int physicalMemory[physicalMemoryBytes];
int pageFault = 0;
*/

#define TLB_LENGTH 16
int TLB[TLB_LENGTH][3]; //use 3 to implement the LRU policy

#define PAGE_SIZE 256
int pageTable[PAGE_SIZE][3]; //use 3 to implement the LRU policy

#define FRAMELENGTH 256
char readBacker[FRAMELENGTH];

#define FRAMES_LENGTH 256
#define OFFSET_LENGTH 256
int physicalMemory[FRAMES_LENGTH][OFFSET_LENGTH];

#define PGROUNDUP(sz)  (((sz)+PAGE_SIZE-1) & ~(PAGE_SIZE-1))
#define PGROUNDDOWN(a) (((a)) & ~(PAGE_SIZE-1))

void updateTLBcounter(int access)
{
	for (int i = 0; i < TLB_LENGTH; i++)
	{
		if (i == access)
		{
			TLB[i][2]++;
		}
		else
		{
			TLB[i][2]--;
		}
	}
}

void updatePageTablecounter(int access)
{
	for (int i = 0; i < PAGE_SIZE; i++)
	{
		if (i == access)
		{
			pageTable[i][2]++;
		}
		else
		{
			pageTable[i][2]--;
		}
	}
}

void insertTLB(int pageNumber, int frameNumber)
{
	int largest_value = TLB[0][2];
	int min = 0;
	int min_value = TLB[0][2];

	int i;

	for (i = 0; i < TLB_LENGTH; i++)
	{
		if (TLB[i][2] < min_value)
		{
			min = i;
			min_value = TLB[i][2];
		}

		if (TLB[i][2] > largest_value)
		{
			largest_value = TLB[i][2];
		}
	}

	TLB[min][0] = pageNumber;
	TLB[min][1] = frameNumber;
	TLB[min][2] = largest_value + 1;
}

void insertPageTable(int pageNumber, int frameNumber)
{
	int largest_value = pageTable[0][2];
	int min = 0;
	int min_value = pageTable[0][2];

	int i;

	for (i = 0; i < PAGE_SIZE; i++)
	{
		if (pageTable[i][2] < min_value)
		{
			min = i;
			min_value = pageTable[i][2];
		}

		if (pageTable[i][2] > largest_value)
		{
			largest_value = pageTable[i][2];
		}
	}

	pageTable[min][0] = pageNumber;
	pageTable[min][1] = frameNumber;
	pageTable[min][2] = largest_value + 1;
}

int read_physical_memory(int frameNumber, int offset)
{
	int value;
	if ((frameNumber < 256) && (offset < 256))
	{
		value = physicalMemory[frameNumber][offset];
		return value;
	}
	else
	{
		printf("Frame number or offset is not bound!");
		return -1;
	}
}

char *decimal_to_binary(int n, int limit)
{
	int c, d, t;
	char *p;

	t = 0;
	p = (char*)malloc(limit);

	if (p == NULL)
	{
		exit(EXIT_FAILURE);
	}

	for (c = limit - 1; c >= 0 ; c--)
	{
		d = n >> c;

		if (d & 1)
		{
			*(p+t) = '1';
		}
		else
		{
			*(p+t) = '0';
		}

		t++;
	}

	return  p;
}

int binary_to_decimal(char* binary_string, int length)
{
	char* start = &binary_string[0];
	int total = 0;
	while (*start)
	{
		total *= 2;
		if (*start++ == '1') total += 1;
	}

	return total;
}

int check_tlb(int pageNumber, int offset, int virtual_address)
{
	int frameNumber;
	int value;

	char* phys_frame;
	char* phys_offset;
	char* phys_address_char;
	int physical_address;

	int frame_size = 8;
	int offset_size = 8;
	int total_size = 16;

	for (int i = 0; i < TLB_LENGTH; i++)
	{
		if (pageNumber == TLB[i][0])
		{
			printf("Page Number %d Found in TLB\n", pageNumber);

			frameNumber = TLB[i][1];

			value = read_physical_memory(frameNumber, offset);

			printf("The value is %d\n", value);

			phys_frame = decimal_to_binary(frameNumber, frame_size);

			phys_offset = decimal_to_binary(offset, offset_size);

			printf("The frame binary is %s\n", phys_frame);

			printf("The offset binary is %s\n", phys_offset);

			phys_address_char = strcat(phys_frame, phys_offset);

			printf("The physical address is %s\n", phys_address_char);

			physical_address = binary_to_decimal(phys_address_char, total_size);

			printf("The virtual address is %d and the physical address is %d\n", virtual_address, physical_address);

			updateTLBcounter(i);

			return 1;
		}
	}

	return -1;
}

int check_page_table(int pageNumber, int virtual_address, int offset)
{
	int frameNumber;
	int value;

	char* phys_frame;
	char* phys_offset;
	char* phys_address_char;
	int physical_address;

	int frame_size = 8;
	int offset_size = 8;
	int total_size = 16;

	for (int i = 0; i < PAGE_SIZE; i++)
	{
			if (pageNumber == pageTable[i][0])
			{
				printf("Page Number %d Found in Page Table\n", pageNumber);

				frameNumber = pageTable[i][1];

				value = read_physical_memory(frameNumber, offset);

				printf("The value is %d\n", value);

				phys_frame = decimal_to_binary(frameNumber, frame_size);

				phys_offset = decimal_to_binary(offset, offset_size);

				printf("The frame binary is %s\n", phys_frame);

				printf("The offset binary is %s\n", phys_offset);

				phys_address_char = strcat(phys_frame, phys_offset);

				printf("The physical address is %s\n", phys_address_char);

				physical_address = binary_to_decimal(phys_address_char, total_size);

				printf("The virtual address is %d and the physical address is %d\n", virtual_address, physical_address);

				updatePageTablecounter(i);

				insertTLB(pageNumber, frameNumber);

				return 1;
			}
		}

		return -1;
}

void page_fault_handler(int pageNumber)
{
	if (pageNumber < 256)
	{
		if(fseek(backStore, pageNumber * PAGE_SIZE, SEEK_SET) != 0)
		{
			printf("ERROR\n");
		}

		if(fread(readBacker, sizeof(signed char), PAGE_SIZE, backStore) == 0) {
			printf("ERROR\n");
		}

		int i;
		int availableFrame;

		for(i =0; i < PAGE_SIZE; i++)
		{
			if((pageTable[i][0] == 0) && (pageTable[i][1] == 0)) {
					availableFrame = i;
					break;
			}
		}

		for(int m = 0; m < PAGE_SIZE; m++) {
			physicalMemory[availableFrame][m] = readBacker[m];
		}

		insertTLB(pageNumber, availableFrame);
		insertPageTable(pageNumber, availableFrame);

		//printf("HERE");
	}
	else
	{
		printf("Page Number is Out of Bound!\n");
		return;
	}
}

void map_page(int pageNumber, int values[], int length)
{
	if (pageNumber < 256)
	{
		int availableFrame;

		int largest_value = pageTable[0][2];
		int min = 0;
		int min_value = pageTable[0][2];

		int i;

		for (i = 0; i < PAGE_SIZE; i++)
		{
			if (pageTable[i][2] < min_value)
			{
				min = i;
				min_value = pageTable[i][2];
			}

			if (pageTable[i][2] > largest_value)
			{
				largest_value = pageTable[i][2];
			}
		}

		availableFrame = min;

		//pageTable[min][0] = pageNumber;
		//pageTable[min][1] = frameNumber;
		//pageTable[min][2] = largest_value + 1;

		for(int m = 0; m < length; m++) {
			physicalMemory[availableFrame][m] = values[m];
		}

		insertTLB(pageNumber, availableFrame);
		//insertPageTable(pageNumber, availableFrame);

		pageTable[availableFrame][0] = pageNumber;
		pageTable[availableFrame][1] = availableFrame;
		pageTable[availableFrame][2] = largest_value + 1;

		//printf("HERE");
	}
	else
	{
		printf("Page Number is Out of Bound!\n");
		return;
	}
}

void *mmap(int addr, size_t length, char* file)
{
	FILE* addressFile = fopen(file, "r");

	if(addressFile == NULL) {
		printf("Cannot open file");
		return 1;
	}

	int values[length];
	int i = 0;

	for (i = 0; i < length; i++)
	{
		fscanf(addressFile, "%d\n", &values[i]);
	}

	int offset;
	int pageOriginal;
	int pageNumber;

	printf("Mapping a File!\n");

	printf("Virtual address is %d\n", addr);

	offset = addr & 255;

	printf("%d\n", offset);

	pageOriginal = addr & 65280;

	printf("%d\n", pageOriginal);

	pageNumber = pageOriginal >> 8;

	map_page(pageNumber, values, length);
}

int main()
{
	addressFile = fopen("addresses.txt", "r");
	backStore = fopen("BACKING_STORE.bin", "r");

	if(addressFile == NULL) {
		printf("Cannot open file");
		return 1;
	}

	if(backStore == NULL) {
		printf("Cannot open file");
		return 1;
	}

	char* line;
	int virtual_addresses[1000];
	int i = 0;

	for (i = 0; i < 1000; i++)
	{
		fscanf(addressFile, "%d\n", &virtual_addresses[i]);
	}

	TLB[0][0] = 122;
	TLB[0][1] = 100;

	pageTable[0][0] = 38;
	pageTable[0][1] = 100;

	int offset;
	int pageOriginal;
	int pageNumber;
	int tlb_hit;
	int tlb_hit_counter = 0;

	int page_table_hit;
	int page_table_hit_counter = 0;

	int page_fault_counter = 0;

	for (i = 0; i < 1000; i++)
	{
		printf("%d\n", virtual_addresses[i]);

		offset = virtual_addresses[i] & 255;

		printf("%d\n", offset);

		pageOriginal = virtual_addresses[i] & 65280;

		printf("%d\n", pageOriginal);

		pageNumber = pageOriginal >> 8;

		printf("%d\n\n", pageNumber);

		tlb_hit = check_tlb(pageNumber, offset, virtual_addresses[i]);

		if (tlb_hit == 1)
		{
			tlb_hit_counter++;
		}
		else
		{
			page_table_hit = check_page_table(pageNumber, virtual_addresses[i], offset);

			if (page_table_hit == -1)
			{
				printf("Page not found in TLB or page table!\n");

				page_fault_counter++;

				page_fault_handler(pageNumber);
			}
			else
			{
				page_table_hit_counter++;
			}
		}
	}

	fclose(addressFile);
	fclose(backStore);

	float tlb_hit_rate = tlb_hit_counter / 1000.0;
	float page_hit_rate = page_table_hit_counter / 1000.0;
	float page_fault_rate = page_fault_counter / 1000.0;

	printf("TLB hit rate is: %f\n", tlb_hit_rate);
	printf("Page hit rate is: %f\n", page_hit_rate);
	printf("Page fault rate is: %f\n", page_fault_rate);

	mmap(virtual_addresses[1], 8, "input_file.txt");

	return 0;
}
