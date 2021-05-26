/*
 * main.c
 *
 *  Created on: 26 May 2021
 *      Author: Grant
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#define LENGTH 15

int main()
{
	int p[2];

	pipe(p);

	for (int i = 2; i < 21; i++)
	{
		int pid = fork();

		if (pid > 0) {
			char* pipe_input;
			sprintf(pipe_input, "%d", i);
			write(p[1], pipe_input, LENGTH);


			wait((int *) 0);
			close(p[0]);
			close(p[1]);
		} else if (pid == 0){
			wait((int *) pid);

			char* child_buffer;
			read(p[0], child_buffer, LENGTH);

			int a = 1;
			for (int j = 2; j < i; j++)
			{
				if (i % j == 0)
				{
					a = 0;
				}
			}

			if (a == 1)
			{
			printf("prime %s\n", child_buffer);
			}

		} else {
			printf("fork error\n");
		}
	}
}
