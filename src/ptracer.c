#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ptrace.h>

static int
usage(FILE *fp,const char *a0){
	int t = 0,r;

	if((r = fprintf(fp,"usage: %s -- cmd [ args ]\n",a0)) < 0){
		return r;
	}
	t += r;
	return t;
}

static int
launch(char * const *argv){
	pid_t p;

	if((p = fork()) < 0){
		fprintf(stderr,"Error forking (%s)\n",strerror(errno));
	}else if(p == 0){
		if(ptrace(PTRACE_TRACEME,0,0,0)){
			fprintf(stderr,"Error invoking ptrace (%s)\n",strerror(errno));
		}else if(execvp(*argv,argv)){
			fprintf(stderr,"Error execing %s (%s)\n",*argv,strerror(errno));
		}
	}else{
		do{
			int status,r;

			while((r = waitpid(p,&status,0)) < 0){
				if(errno != EINTR){
					fprintf(stderr,"Error waiting for %ju (%s)\n",
							(uintmax_t)p,strerror(errno));
					return -1;
				}
			}
			if(!WIFSTOPPED(status)){
				return 0;
			}
		}while(ptrace(PTRACE_SINGLESTEP,p,0,0) == 0);
		fprintf(stderr,"Error ptracing %ju (%s)\n",(uintmax_t)p,strerror(errno));
	}
	return -1;
}

static int
parse_args(int argc,char * const *argv){
	if(argc < 3){
		usage(stderr,*argv);
		return -1;
	}
	return 0;
}

int main(int argc,char **argv){
	if(parse_args(argc,argv)){
		return EXIT_FAILURE;
	}
	argv += 2; // FIXME handle options
	if(launch(argv)){
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
