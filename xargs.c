#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#define MAXLINE 1000
#define MAXARGS 128

void unix_error(char *msg) /* Unix-style error */
{
 fprintf(stderr, "%s: %s\n", msg, strerror(errno));
 exit(0);
}

int builtin_command(char **argv) 
{
    if (!strcmp(argv[0], "quit")) /* quit command */
	exit(0);  
    if (!strcmp(argv[0], "&"))    /* Ignore singleton & */
	return 1;
    return 0;                     /* Not a builtin command */
}

int parseline(char *buf, char **argv) 
{
    char *delim;         /* Points to first space delimiter */
    int argc;            /* Number of args */
    int bg;              /* Background job? */

    buf[strlen(buf)-1] = ' ';  /* Replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* Ignore leading spaces */
	buf++;

    /* Build the argv list */
    argc = 0;
    while ((delim = strchr(buf, ' '))) {
	argv[argc++] = buf;
	*delim = '\0';
	buf = delim + 1;
	while (*buf && (*buf == ' ')) /* Ignore spaces */
	       buf++;
    }
    argv[argc] = NULL;
    
    if (argc == 0)  /* Ignore blank line */
	return 1;

    /* Should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0)
	argv[--argc] = NULL;

    return bg;
}

void eval(char *cmdline)
{
 	char *argv[MAXARGS]; /* Argument list execve() */ 
	char buf[MAXLINE]; /* Holds modified command line */ 
	int bg; /* Should the job run in bg or fg? */
 
	pid_t pid; /* Process id */

 	strcpy(buf, cmdline);
 	bg = parseline(buf, argv); 
	
	if (argv[0] == NULL) 
		return; /* Ignore empty lines */
	
 	if (!builtin_command(argv))
	{		
		if ((pid = fork()) == 0) 
		{ /* Child runs user job */  
			if (execve(argv[0], argv, NULL) < 0)
			{ 
				printf("%s: Command not found.\n", argv[0]);
 				exit(0);
 			}
 		} 

 		/* Parent waits for foreground job to terminate */
		if (!bg) { 
			int status;
 
			if (waitpid(pid, &status, 0) < 0) 
				unix_error("waitfg: waitpid error");
 		}
		else 
			printf("%d %s", pid, cmdline);
 	}	

 	return;
}

int main()
{
	char cmdline[MAXLINE]; /* command line */
 	
	while (1) {
		/* read */
 		printf("> ");
 		fgets(cmdline, MAXLINE, stdin);
 		
		if (feof(stdin))
 			exit(0);
 
		/* evaluate */
 		eval(cmdline);
 	}
}