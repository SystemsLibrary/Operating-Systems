#include <stdio.h>
#include <stdlib.h> 
#include <signal.h> // sigaction(), sigsuspend(), sig*()
#include <unistd.h> // alarm()

void handler(int signal)
{
	char* signal_name;
	sigset_t p;

	switch (signal)
	{
		case SIGHUP:
			signal_name = "SIGHUP";
			break;
		case SIGABRT:
			printf("Caught SIGABRT, exiting now\n");
			exit(0);
		default:
			printf("Caught wrong signal: %d\n", signal);
            		return;
	}

	printf("Caught %s\n", signal_name);
}

int main() {
    	struct sigaction sa;

	printf("My pid is: %d\n", getpid());

    	sa.sa_handler = &handler;

    	sa.sa_flags = SA_RESTART;

    	sigfillset(&sa.sa_mask);

	if (sigaction(SIGHUP, &sa, NULL) == -1) {
        	perror("Error: cannot handle SIGHUP"); // Should not happen
    	}

	if (sigaction(SIGABRT, &sa, NULL) == -1) {
        	perror("Error: cannot handle SIGINT"); // Should not happen
    	}

	if (sigaction(SIGINT, &sa, NULL) == -1) {
        	perror("Error: cannot handle SIGINT"); // Should not happen
    	}
	
	while(3)
	{
		printf("\nSleeping for ~3 seconds\n");
		sleep(3);
	}
}
