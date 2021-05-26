/*
 * main.c
 *
 *  Created on: 26 May 2021
 *      Author: Grant
 */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	int time = atoi(argv[1]);

    sleep(time);

    exit(0);
}
