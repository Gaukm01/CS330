#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/wait.h>

//not used because of using sprintf()
void itostring (long long n, char* rev){
	char ans[22];
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
	//printf("entered ");
	// if(argv[argc-1]!=NULL){
	// 	printf("entered added null");
	// 	char* argv_new[argc+1];
	// 	for(int i=0;i<argc;i++) {
	// 		argv_new[i]=argv[i];
	// 	}
	// 	argv_new[argc]=NULL;
	// 	printf("entered added null");
	// 	//execv("./double",argv_new);
	// 	//exit(0);
	// }

	//printf("in double function argc:%d argv[argc-1]: %s\n",argc,argv[argc-1]);
	unsigned long long int n=atoi(argv[argc-1]);
	double val=n;
	val=2*val; // double the value
	sprintf(argv[argc-1],"%llu",(unsigned long long int)val); // update the value at the end of arguments
	if(argc <= 2) {  //2 arguments means give output
		double out= round(val);
		printf("%llu\n",(unsigned long long int)out);
	}
	else { // else call exec

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
		if(execv(buffer,argv+1)){
			printf("UNABLE TO EXECUTE\n");
			exit(-1); // exit out of execution
		}
		//printf("still here\n");
	}
	return 0;
}
