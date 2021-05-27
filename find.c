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
#include <dirent.h>

void find(char* current_dir, char* search)
{
	char temp[500];
	DIR *dp = NULL;
	struct dirent *dptr = NULL;

	dp = opendir(current_dir);

	while(NULL != (dptr = readdir(dp)))
	{
		if(!strcmp (dptr->d_name, "."))
		{
			continue;
		}
		if(!strcmp (dptr->d_name, ".."))
		{
			continue;
		}

		if(!strcmp (dptr->d_name, search) && dptr->d_type != 4)
		{
			printf("Found file at directory: %s\n", current_dir);
			return;
		}

		getcwd(temp, 100);

		if (dptr->d_type == 4)
		{
			strcat(temp, "/");
			strcat(temp, dptr->d_name);
		    find(temp, search);
		}
	}

	closedir(dp);
}

int main()
{
	char* search = "makefile";

	char* current_dir;

	getcwd(current_dir, 100);

	find(current_dir, search);

	return 0;
}
