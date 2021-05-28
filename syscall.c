/*
 * syscall.c
 *
 *  Created on: 28 May 2021
 *      Author: Grant
 */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/prctl.h>
#include <sys/user.h>

int main(int argc, char *argv[])
{
	int w;
	int status;
	struct user regs;

	int pid = fork();

	if (pid > 0) {
		while(1) {
            		w = waitpid(pid, &status, WUNTRACED | WCONTINUED);
            
			if (w == -1) {
                		perror("waitpid");
                		exit(1);
            		}

           		if (WIFEXITED(status)) {
                		printf("exited, status=%d\n", WEXITSTATUS(status));
				break;
            		} else if (WIFSIGNALED(status)) {
                		printf("killed by signal %d\n", WTERMSIG(status));
				break;
            		} else if (WIFSTOPPED(status)) {
				ptrace(PTRACE_GETREGS, pid, (void *)0, &regs);
				if (regs.regs.rax <= 335)
				{
					printf("stopped by signal %d\n", WSTOPSIG(status));	
					printf("Syscall number is: %llu\n", regs.regs.rax);
				}
				ptrace(PTRACE_SYSCALL, pid, (void *)0, (void *)0);
            		} else if (WIFCONTINUED(status)) {
                		printf("continued\n");
            		}

        	}
        	exit(0);
	} else if (pid == 0){
		prctl(PR_SET_DUMPABLE, 1L);
		ptrace(PTRACE_TRACEME, (pid_t)0, (void *)0, (void *)0);
		ptrace(PTRACE_SETOPTIONS, getpid(), PTRACE_O_TRACECLONE | PTRACE_O_TRACEEXEC | PTRACE_O_TRACEEXIT | PTRACE_O_TRACEFORK);
		//raise(SIGSTOP);		

		if (execvp(argv[1], argv + 1) < 0)
		{
			exit(127);
		}
		
	} else {
		printf("fork error\n");
	}

    exit(0);
}
