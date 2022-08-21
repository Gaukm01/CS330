#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/wait.h>

//not used because of using sprintf()
void itostring (long long n, char* rev){
	char ans[22];//rev[22];
	int i=0;
	while(n>0){
		ans[i++]= '0'+(n%10);
		n=n/10;
	}
	for(int j=i-1,k=0;j>=0;j--,k++){
		rev[k]=ans[j];
	}
	rev[i]='\0';
}

int main(int argc, char *argv[])
{	
	unsigned long long int n=atoi(argv[argc-1]);
	double val=n;
	val=round(sqrt(n)); // root value and round it
	sprintf(argv[argc-1],"%llu",(unsigned long long int)val); // update the value at the end of arguments
	if(argc <= 2) {
		double out= round(val);
		printf("%llu\n",(unsigned long long int)out);
	}
	else {
		// char *next_func= argv[1];
		// long long int n=atoi(argv[argc-2]);		
		// char* next_args[argc-1];
		// for(int i=1;i<argc;i++) {
		// 	next_args[i-1]=argv[i];
		// }
		// char temp1[22];
		// itostring(n,temp1);
		// next_args[argc-3]=temp1;

		char  buffer[20] = "./", *temp = argv[1];//next_args[0];
		strcat(buffer, temp);		
		if(execv(buffer,argv+1)){ // if not executed return -1 enters inside
			printf("UNABLE TO EXECUTE\n");
			exit(-1); // exit out of execution
		}
		//printf("still here\n");
	}
	return 0;
}
