/*
 * main.c
 *
 *  Created on: 26 May 2021
 *      Author: Grant
 */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#define LENGTH 15

int main()
{
	int p[2];

	char* child_buffer;
	char* parent_buffer;

	pipe(p);
	int pid = fork();

	if (pid > 0) {
		char* parent_pid;
		sprintf(parent_pid, "%d", pid);
		write(p[1], parent_pid, LENGTH);
		wait((int *) 0);
		read(p[0], parent_buffer, LENGTH);
		printf("%s: received pong\n", parent_buffer);
		close(p[0]);
		close(p[1]);
	} else if (pid == 0){
		read(p[0], child_buffer, LENGTH);
		printf("%s: received ping\n", child_buffer);
		write(p[1], child_buffer, LENGTH);
		exit(0);
	} else {
		printf("fork error\n");
	}

    exit(0);
}
