#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <libdis.h>
#include <sys/user.h>
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
decode(pid_t pid,uintmax_t ip,uintmax_t *oldip){
	unsigned buf[4],s;
	x86_insn_t instr;

	buf[0] = ptrace(PTRACE_PEEKTEXT,pid,ip,0);
	buf[1] = ptrace(PTRACE_PEEKTEXT,pid,ip + sizeof(unsigned),0);
	buf[2] = ptrace(PTRACE_PEEKTEXT,pid,ip + 2 * sizeof(unsigned),0);
	buf[3] = ptrace(PTRACE_PEEKTEXT,pid,ip + 3 * sizeof(unsigned),0);
	s = x86_disasm((unsigned char *)buf,sizeof(buf),0,0,&instr);
	if(ip <= *oldip){
		printf("%012lx] (%u) -0x%lx\n",ip,s,*oldip - ip);
	}else{
		printf("%012lx] (%u) 0x%lx\n",ip,s,ip - *oldip);
	}
	*oldip = ip;
	return 0;
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
		uintmax_t ops = 0,rip = 0;

		do{
			struct user_regs_struct regs;
			int status,r;

			while((r = waitpid(p,&status,0)) < 0){
				if(errno != EINTR){
					fprintf(stderr,"Error waiting for %ju (%s)\n",
							(uintmax_t)p,strerror(errno));
					return -1;
				}
			}
			if(!WIFSTOPPED(status)){
				printf("Counted %ju instructions.\n",ops);
				return 0;
			}
			++ops;
			if(ptrace(PTRACE_GETREGS,p,0,&regs)){
				break;
			}
			if(decode(p,regs.rip,&rip)){
				return -1;
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
	int r;

	if(parse_args(argc,argv)){
		return EXIT_FAILURE;
	}
	if((r = x86_init(opt_none,NULL,NULL)) != 1){
		fprintf(stderr,"Couldn't initialize libdisasm (%d)\n",r);
		return EXIT_FAILURE;
	}
	argv += 2; // FIXME handle options
	if(launch(argv)){
		return EXIT_FAILURE;
	}
	if((r = x86_cleanup()) != 1){
		fprintf(stderr,"Couldn't clean up libdisasm (%d)\n",r);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
