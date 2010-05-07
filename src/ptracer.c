#include <stdio.h>
#include <stdlib.h>

static int
usage(FILE *fp,const char *a0){
	int t = 0,r;

	if((r = fprintf(fp,"usage: %s -- cmd [ args ]\n",a0)) < 0){
		return r;
	}
	t += r;
	return t;
}

int main(int argc,char **argv){
	if(argc != 1){
		usage(stderr,*argv);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
