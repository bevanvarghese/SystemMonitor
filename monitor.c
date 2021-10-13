#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/time.h>
#include <sys/resource.h>
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
			printf("-----\n");

			printf("Process with id: %d created for the command: %s\n\n", getpid(), argv[current_proc_idx]);

			execvp(argv[current_proc_idx], &argv[current_proc_idx]);

			printf("exec: No such file or directory\n");
			printf("\n");
			return EXIT_FAILURE;

		}
		else {

			// parent process: monitor
			printf("Parent Process.\n");
			printf("-----.\n");
			printf("-----.\n");

			// Let monitor ignore incoming interrupt-signals
			// Will not work on SIGKILL and SIGSTOP
			for(int i=1; i<32; i++) {
				signal(i, ignore_signal);
			}

			struct timespec r_start, r_stop;
			struct rusage usage;
			int status;
			clock_gettime(CLOCK_MONOTONIC, &r_start);
			wait4(pid, &status, 0, &usage);
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
			double real_time = (r_stop.tv_sec-r_start.tv_sec) + (double)(r_stop.tv_nsec - r_start.tv_nsec) * 0.000000001;
			double user_time = (double) usage.ru_utime.tv_sec + (double) usage.ru_utime.tv_usec * 0.000001;
			double sys_time = (double) usage.ru_stime.tv_sec + (double) usage.ru_stime.tv_usec * 0.000001;
			int pg_faults = (int) usage.ru_minflt + (int) usage.ru_majflt;  
			int ctxt_switches = (int) usage.ru_nvcsw + (int) usage.ru_nivcsw;  

			printf("\nreal: %.3f s, user: %.3f s, system: %.3f s\n", real_time, user_time, sys_time);
			printf("no. of page faults: %d\n", pg_faults);
			printf("no. of context switches: %d\n", ctxt_switches);
			
			printf("-----.\n");
			printf("Parent Process End.\n");
			printf("\n");

		}

	} 

	return 0;

} // main
