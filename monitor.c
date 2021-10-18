// Filename: monitor_3035552777.c
// Student name: Bevan Varghese
// Student number: 3035552777
// Development platform: WSL (Ubuntu) on Windows 10
/* Remark: All the requirements have been completed. There was no specification mentioning 
that the final output of the command should be printed on the terminal or not. I have 
assumed that the final output should be printed on the terminal. Hence, with one child 
process invoked, the output is printed. When pipes are used, the final output is printed. */

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

// function to ignore signals like SIGINT, SIGSEGV, etc. in the monitor
void ignore_signal(int sig) {
	signal(sig, SIG_IGN);
}

int main(int argc, char *argv[]) {
	

	if(argc==1) {
		// program invoked with no arguments
		// terminate successfully without output
	} 
	else {
		// count number of child processes required
		int num_children = 1;
		for (int i = 1; i < argc; i++)
		{
			if(argv[i][0]=='!') {
				num_children++;
			}
		}

		// 2 cases:
		// (i) pipes not required
		// (ii) pipes required

		if(num_children==1) {
			// pipes not required

			// index of required process
			int current_proc_idx = 1; 
			pid_t pid = fork();

			if(pid<0) {

				// error occurred
				fprintf(stderr, "fork() Failed");
				exit(-1);
			
			}

			else if(pid==0) {

				// child process
				printf("-----\n");
				printf("Process with id: %d created for the command: %s\n\n", getpid(), argv[current_proc_idx]);
				execvp(argv[current_proc_idx], &argv[current_proc_idx]);

				// in case of failure, print and return the corresponding status code
				printf("exec: No such file or directory\n");
				printf("\n");
				return EXIT_FAILURE;
			
			}
			else {

				// parent process: monitor
				printf("Parent Process.\n");
				printf("-----\n");
				printf("-----\n");

				// let the monitor ignore incoming interrupt-signals (codes 1 to 31)
				// will not work on SIGKILL and SIGSTOP
				for(int i=1; i<32; i++) {
					signal(i, ignore_signal);
				}

				struct timespec r_start, r_stop; // track real clock time
				struct rusage usage; // track child's resource usage statistics
				int status; // store child's status/signal code

				// start the clock, wait for the child to finish, stop the clock
				clock_gettime(CLOCK_MONOTONIC, &r_start);
				wait4(pid, &status, 0, &usage);
				clock_gettime(CLOCK_MONOTONIC, &r_stop);

				if(WIFEXITED(status)) {
					if(WEXITSTATUS(status)==EXIT_FAILURE) {
						// child cannot be started 
						printf("monitor experienced an error in starting the command: %s\n", argv[current_proc_idx]);
					}
				}

				if(WIFSIGNALED(status)) {
					// child was interrupted by a signal
					printf("The command \"%s\" is interrupted by the signal number = %d (%s)\n\n", argv[current_proc_idx], WTERMSIG(status), strsignal(WTERMSIG(status)));
				}

				// calculate resource usage statistics with the values saved by wait4()
				double real_time = (r_stop.tv_sec-r_start.tv_sec) + (double)(r_stop.tv_nsec - r_start.tv_nsec) * 0.000000001;
				double user_time = (double) usage.ru_utime.tv_sec + (double) usage.ru_utime.tv_usec * 0.000001;
				double sys_time = (double) usage.ru_stime.tv_sec + (double) usage.ru_stime.tv_usec * 0.000001;
				int pg_faults = (int) usage.ru_minflt + (int) usage.ru_majflt;  
				int ctxt_switches = (int) usage.ru_nvcsw + (int) usage.ru_nivcsw;  

				// print resource usage statistics
				printf("\nreal: %.3f s, user: %.3f s, system: %.3f s\n", real_time, user_time, sys_time);
				printf("no. of page faults: %d\n", pg_faults);
				printf("no. of context switches: %d\n", ctxt_switches);
				printf("-----\n");
				printf("\n");

			}
		} 
		else 
		{
			// pipes required as >1 child processes

			// store the argv[] indices at which each process starts in child_positions[]
			// replace all the '!' with NULL because execvp() takes in argv[] elements until it hits a NULL pointer
			// storing the child processes' indices allows us to start from the required child on each iteration
			int child_positions[num_children];
			child_positions[0] = 1;
			int j = 1;
			for (int i = 1; i < argc; i++)
			{
				if(argv[i][0]=='!') {
					argv[i] = NULL;
					child_positions[j] = i+1;
					j++;
				} 
			}

			// create a pipe for each child process
			// the n-th child will read from the (n-1)th child's pipe
			// the n-th child will write to the n-th child's pipe 
			int fd[num_children*2];
			for(int i = 0; i<num_children; i++) {
				pipe(&fd[2*i]);
			}

			// for-loop: for each child process
			for(int ith_child = 0; ith_child < num_children; ith_child++)
			{

				int current_proc_idx = child_positions[ith_child];
				pid_t pid = fork();

				if(pid<0) {
					// error occurred
					fprintf(stderr, "fork() Failed");
					exit(-1);
				}

				else if(pid==0) {
					// child process
					printf("-----\n");
					printf("Process with id: %d created for the command: %s\n\n", getpid(), argv[current_proc_idx]);

					// for i = 0 to (n-2) children, dup2 the writing end to STDOUT
					// as they feed the output to the next process
					if(ith_child<(num_children-1)) {
						dup2(fd[2*ith_child+1], STDOUT_FILENO);
					} 
					// close the writing end
					close(fd[2*ith_child+1]);

					// for i = 1 to (n-1) children, dup2 the reading end to STDIN
					// as they require the input from the previous process
					if(ith_child>0) {
						dup2(fd[2*(ith_child-1)], STDIN_FILENO);
					} 
					// close the reading end
					close(fd[2*(ith_child-1)]);

					execvp(argv[current_proc_idx], &argv[current_proc_idx]);
					printf("exec: No such file or directory\n");
					printf("-----\n");
					printf("\n");
					return EXIT_FAILURE;
				}

				else  {
					// parent process: monitor

					// monitor does not write to child, so close this pipe's writing end
					close(fd[2*ith_child + 1]); 
					printf("-----\n");

					// generic signal handler for the monitor
					for(int i=1; i<32; i++) {
						signal(i, ignore_signal);
					}

					// resource usage storage variables
					struct timespec r_start, r_stop;
					struct rusage usage;
					int status;
					clock_gettime(CLOCK_MONOTONIC, &r_start);
					wait4(pid, &status, 0, &usage);
					clock_gettime(CLOCK_MONOTONIC, &r_stop);

					// when the i-th child finishes, close the (i-1)th child's reading end
					if(ith_child>0) {
						close(fd[2*(ith_child-1)]);
					} 
					// when the final child finishes, close the final child's reading end
					if(ith_child==num_children-1) {
						close(fd[2*ith_child]);
					}

					// check for abnormal termination
					if(WIFEXITED(status)) {
						if(WEXITSTATUS(status)==EXIT_FAILURE) {
							printf("monitor experienced an error in starting the command: %s\n", argv[current_proc_idx]);
						}
					}

					// check for incoming signals
					if(WIFSIGNALED(status)) {
						printf("The command \"%s\" is interrupted by the signal number = %d (%s)\n\n", argv[current_proc_idx], WTERMSIG(status), strsignal(WTERMSIG(status)));
					}

					// calculate and print resource usage statistics for the i-th child
					double real_time = (r_stop.tv_sec-r_start.tv_sec) + (double)(r_stop.tv_nsec - r_start.tv_nsec) * 0.000000001;
					double user_time = (double) usage.ru_utime.tv_sec + (double) usage.ru_utime.tv_usec * 0.000001;
					double sys_time = (double) usage.ru_stime.tv_sec + (double) usage.ru_stime.tv_usec * 0.000001;
					int pg_faults = (int) usage.ru_minflt + (int) usage.ru_majflt;  
					int ctxt_switches = (int) usage.ru_nvcsw + (int) usage.ru_nivcsw;  
					printf("\nreal: %.3f s, user: %.3f s, system: %.3f s\n", real_time, user_time, sys_time);
					printf("no. of page faults: %d\n", pg_faults);
					printf("no. of context switches: %d\n", ctxt_switches);
					printf("\n");
				}
			}
		}

	} 

	return 0;

} // main
