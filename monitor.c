#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/times.h>
#include <time.h>
#include <signal.h>
#include <string.h>

#define BILLION 1000000000L;

void ignore_signal(int sig) {
	signal(sig, SIG_IGN);
	// printf("\nDeflated\n");
}

int main(int argc, char *argv[]) {
	

	if(argc==1) {
		// terminate successfully without output
	} 
	else {

		int current_proc_idx = 1;
		pid_t pid = fork();
		
		if(pid<0) {
			// Error Occurred
			fprintf(stderr, "fork() Failed");
			exit(-1);
		}

		else if(pid==0) {
			
			// child process
			printf("Child Process.\n");

			printf("Process with id: %d created for the command: %s\n", getpid(), argv[current_proc_idx]);

			execvp(argv[current_proc_idx], &argv[current_proc_idx]);

			printf("exec: No such file or directory\n");
			printf("\n");
			return EXIT_FAILURE;

		}
		else {

			// parent process: monitor
			printf("Parent Process Start.\n");

			// Let monitor ignore incoming interrupt-signals
			// Will not work on SIGKILL and SIGSTOP
			for(int i=1; i<32; i++) {
				signal(i, ignore_signal);
			}

			struct timespec r_start, r_stop;

			int status;
			clock_gettime(CLOCK_MONOTONIC, &r_start);
			waitpid(pid, &status, 0);
			clock_gettime(CLOCK_MONOTONIC, &r_stop);

			// cannot be started 
			if(WIFEXITED(status)) {
				// printf("WEXITSTATUS(status) %d\n", WEXITSTATUS(status));
				if(WEXITSTATUS(status)==EXIT_FAILURE) {
					printf("monitor experienced an error in starting the command: %s\n", argv[current_proc_idx]);
				}
			}
			// signalled
			// TODO: format the string
			if(WIFSIGNALED(status)) {
				printf("The command \"%s\" is interrupted by the signal number = %d (%s)\n\n", argv[current_proc_idx], WTERMSIG(status), strsignal(WTERMSIG(status)));
			}
			
			// http://www.qnx.com/developers/docs/6.5.0/index.jsp?topic=%2Fcom.qnx.doc.neutrino_lib_ref%2Fc%2Fclock_gettime.html
			double realtime = (r_stop.tv_sec-r_start.tv_sec) + (double)(r_stop.tv_nsec - r_start.tv_nsec) / (double)BILLION;

			// printf("real: %.3f s, user: %.3f s, system: %.3f s\n", realtime, usertime, systime);
			printf("\nreal: %.3f s, user: %.3f s, system: %.3f s\n", realtime, 0.0, 0.0);
			
			printf("Parent Process End.\n");
			printf("\n");

		}

	} 

	return 0;

} // main
