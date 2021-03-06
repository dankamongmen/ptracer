#include <stdio.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <getopt.h>
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

#if __WORDSIZE == 64
#define IP(regs) regs.rip
#else
#define IP(regs) regs.eip
#endif

static int
decode(pid_t pid,unsigned long ip){
	unsigned char buf[16],z;
	x86_insn_t instr;
	Dl_info dinfo;
	char line[80];
	int s,r,ugh;

	buf[0] = ptrace(PTRACE_PEEKTEXT,pid,ip,0);
	buf[1] = ptrace(PTRACE_PEEKTEXT,pid,ip + sizeof(unsigned),0);
	buf[2] = ptrace(PTRACE_PEEKTEXT,pid,ip + 2 * sizeof(unsigned),0);
	buf[3] = ptrace(PTRACE_PEEKTEXT,pid,ip + 3 * sizeof(unsigned),0);
	s = x86_disasm((unsigned char *)buf,sizeof(buf),0,0,&instr);
	if(s){
		x86_format_insn(&instr,line,sizeof(line),intel_syntax);
	}else{
		line[0] = '\0';
	}
	if((r = dladdr((void *)ip,&dinfo)) == 0){
		r = printf("%012lx] (%x)%n %s\n",ip,s,&ugh,line);
	}else{
		if(dinfo.dli_sname && ip == (unsigned long)dinfo.dli_saddr){
			r = printf("%012lx:%s] (%x)%n %s\n",ip,dinfo.dli_sname,s,&ugh,line);
		}else if(dinfo.dli_sname){
			r = printf("%lx:%s] (%x)%n %s\n",ip - (unsigned long)dinfo.dli_saddr,
					dinfo.dli_sname,s,&ugh,line);
		}else{
			const char *sl;

			if( (sl = strrchr(dinfo.dli_fname,'/')) ){
				++sl;
			}else{
				sl = dinfo.dli_fname;
			}
			r = printf("%012lx:%s] (%x)%n %s\n",ip,sl,s,&ugh,line);
		}
	}
	if(s){
		for(z = 0 ; z < ugh ; ++z){
			putc_unlocked(' ',stdout);
		}
		for(z = 0 ; z < s ; ++z){
			printf(" %02x ",buf[z]);
		}
		printf("\n");
	}
	return 0;
}

static int
launch(char * const *argv){
	pid_t p;

	if(fflush(stdout) || fflush(stderr)){
		fprintf(stderr,"Error flushing output prior to fork (%s)\n",strerror(errno));
		return -1;
	}
	if((p = fork()) < 0){
		fprintf(stderr,"Error forking (%s)\n",strerror(errno));
	}else if(p == 0){
		if(ptrace(PTRACE_TRACEME,0,0,0)){
			fprintf(stderr,"Error invoking ptrace (%s)\n",strerror(errno));
		}else if(execvp(*argv,argv)){
			fprintf(stderr,"Error execing %s (%s)\n",*argv,strerror(errno));
		}
	}else{
		uintmax_t ops = 0;

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
			if(decode(p,IP(regs))){
				return -1;
			}
		}while(ptrace(PTRACE_SINGLESTEP,p,0,0) == 0);
		fprintf(stderr,"Error ptracing %ju (%s)\n",(uintmax_t)p,strerror(errno));
	}
	return -1;
}

#define OPTSTR "h"
static int
parse_args(int *argc,char * const *argv){
	const struct option longopts[] = {
		{ "help",	0,	NULL,	'h'	},
	};
	int lidx,r;

	opterr = 0; // preclude error diagnostics from getopt()
	while((r = getopt_long(*argc,argv,OPTSTR,longopts,&lidx)) >= 0){
		switch(r){
			case 'h':
				usage(stdout,argv[0]);
				exit(EXIT_SUCCESS);
			default:
			case '?':
				fprintf(stderr,"Unknown option: %c\n",optopt);
				usage(stderr,argv[0]);
				return -1;
		}
	}
	return 0;
}
#undef OPTSTR

int main(int argc,char **argv){
	int r;

	if(parse_args(&argc,argv)){
		return EXIT_FAILURE;
	}
	if(argv[optind] == NULL){
		usage(stderr,*argv);
		return -1;
	}
	if((r = x86_init(opt_none,NULL,NULL)) != 1){
		fprintf(stderr,"Couldn't initialize libdisasm (%d)\n",r);
		return EXIT_FAILURE;
	}
	if(launch(argv + optind)){
		return EXIT_FAILURE;
	}
	if((r = x86_cleanup()) != 1){
		fprintf(stderr,"Couldn't clean up libdisasm (%d)\n",r);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
