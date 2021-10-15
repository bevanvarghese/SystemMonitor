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
		// count number of child processes required
		int num_children = 1;
		for (int i = 1; i < argc; i++)
    {
			if(argv[i][0]=='!') {
				num_children++;
			} 
    }

		// single child process without pipes
		if(num_children==1) {
			int current_proc_idx = 1;
			pid_t pid = fork();
			if(pid<0) {
				// error occurred
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
				printf("-----\n");
				printf("-----\n");
				// let monitor ignore incoming interrupt-signals
				// will not work on SIGKILL and SIGSTOP
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
				printf("-----\n");
				printf("\n");
			}
		} else {
			// pipes required
			// store the positions at which each process starts
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

			int fd[num_children*2];
			for(int i = 0; i<num_children; i++) {
				pipe(&fd[2*i]);
			}

			for(int ith_child = 0; ith_child < num_children; ith_child++)
			{
				int current_proc_idx = child_positions[ith_child];
				// printf("%s\n", argv[current_proc_idx]);
				// int fd[2];
				// if (pipe(fd) == -1) {
				// 	printf("Unable to create pipe\n");
				// 	return 1;
				// }

				pid_t pid = fork();
				if(pid<0) {
					// error occurred
					fprintf(stderr, "fork() Failed");
					exit(-1);
				}
				else if(pid==0) {
					// child process
					// signal(SIGPIPE, SIG_IGN);
					printf("-----\n");
					printf("Process with id: %d created for the command: %s\n\n", getpid(), argv[current_proc_idx]);
					if(ith_child<(num_children-1)) {
						dup2(fd[2*ith_child+1], STDOUT_FILENO);
					} 
					close(fd[2*ith_child+1]);
					if(ith_child>0) {
						dup2(fd[2*(ith_child-1)], STDIN_FILENO);
					} 
					close(fd[2*(ith_child-1)]);
					execvp(argv[current_proc_idx], &argv[current_proc_idx]);
					printf("exec: No such file or directory\n");
					printf("-----\n");
					printf("\n");
					return EXIT_FAILURE;
				}
				else  {
					// parent process: monitor
					close(fd[2*ith_child + 1]);
					printf("-----\n");
					// let monitor ignore incoming interrupt-signals
					// will not work on SIGKILL and SIGSTOP
					for(int i=1; i<32; i++) {
						signal(i, ignore_signal);
					}
					struct timespec r_start, r_stop;
					struct rusage usage;
					int status;
					clock_gettime(CLOCK_MONOTONIC, &r_start);
					wait4(pid, &status, 0, &usage);
					clock_gettime(CLOCK_MONOTONIC, &r_stop);
					if(ith_child>0) {
						close(fd[2*(ith_child-1)]);
					} 
					if(ith_child==num_children-1) {
						close(fd[2*ith_child]);
					}
					// cannot be started 
					if(WIFEXITED(status)) {
						// printf("WEXITSTATUS(status) %d\n", WEXITSTATUS(status));
						if(WEXITSTATUS(status)==EXIT_FAILURE) {
							printf("monitor experienced an error in starting the command: %s\n", argv[current_proc_idx]);
						}
					}
					// signalled
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
					printf("\n");
				}
			}
		}

	} 

	return 0;

} // main
