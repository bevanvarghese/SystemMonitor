#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/times.h>
#include <time.h>

#define BILLION 1000000000L;

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
			// exit(-1);
			return EXIT_FAILURE;

		}
		else {

			// Parent Process
			printf("Parent Process Start.\n");

			// clock_t starttime = clock();
			struct timespec r_start, r_stop;
			// clock_t u_start, u_stop;

			// struct tms t;
			// struct tms t1, t2;
			// long tics_per_second = sysconf(_SC_CLK_TCK);
			// printf("%d\ntics\n", tics_per_second);

			int status;
			clock_gettime(CLOCK_MONOTONIC, &r_start);
			// u_start = clock();
			// if ((u_start = times (&t1)) == -1)     
      //   perror ("times");
			waitpid(pid, &status, 0);
			clock_gettime(CLOCK_MONOTONIC, &r_stop);
			// u_stop = clock();
			// if ((u_stop = times (&t2)) == -1)     
      //   perror ("times");

			if(WIFEXITED(status)) {
				int es = WEXITSTATUS(status);
				// printf("es%d\n", es);
				if(es==EXIT_FAILURE) {
					printf("monitor experienced an error in starting the command: %s\n", argv[current_proc_idx]);
				}
			}
			
			// http://www.qnx.com/developers/docs/6.5.0/index.jsp?topic=%2Fcom.qnx.doc.neutrino_lib_ref%2Fc%2Fclock_gettime.html
			double realtime = (r_stop.tv_sec-r_start.tv_sec) + (double)(r_stop.tv_nsec - r_start.tv_nsec) / (double)BILLION;
			// double usertime = ((double) u_stop - u_start)/CLOCKS_PER_SEC;
			// printf("%f\n", (double) u_stop);
			// printf("%f\n", (double) u_start);
			// double usertime = (double) (t.tms_cutime)/tics_per_second;
			// double systime = (double) (t.tms_cstime)/tics_per_second;
			// printf ("u_start = %ld, times: %ld %ld %ld %ld\n", u_start, t1.tms_utime,  
      //   t1.tms_cutime, t1.tms_stime, t1.tms_cstime);     
    	// printf ("u_stop = %ld, times: %ld %ld %ld %ld\n", u_stop, t2.tms_utime,     
      //   t2.tms_cutime, t2.tms_stime, t2.tms_cstime);     
    	// printf ("u_stop - u_start = %ld\n", u_stop - u_start); 
			// printf ("t2.user - t1.user = %ld\n", t2.tms_cutime - t1.tms_cutime); 
			// printf ("t2.sys - t1.sys = %ld\n", t2.tms_cstime - t1.tms_cutime);
			// printf ("real: %.3f s\n", realtime);

			// printf("real: %.3f s, user: %.3f s, system: %.3f s\n", realtime, usertime, systime);
			printf("\nreal: %.3f s, user: %.3f s, system: %.3f s\n", realtime, 0.0, 0.0);
			
			printf("Parent Process End.\n");
			printf("\n");
		}

	} 

	return 0;

} // main
