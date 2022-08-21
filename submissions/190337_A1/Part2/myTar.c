#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h> 
#include <sys/stat.h>
#include <fcntl.h>


void create_tar(int argc, char *argv[]){
	//if(strcmp(argv[1],"-c")!=0) return;
	if(argc!=4){
	    printf("Failed to complete creation operation");
	    exit(-1);
	}
	//char* tar_dir=argv[3];
	unsigned long long tar_totsize=0;
	unsigned long long filesize=0;
	int no_files=0;
    
    DIR *dirctry=opendir(argv[2]);//("."); // link to mentioned directory
	chdir(argv[2]); // cd to mentiond dir
    struct dirent *entries; 
    struct stat size;
		char tarfolder_path[100];
		sprintf(tarfolder_path,"%s/%s",argv[2],argv[3]);
        int tar_fd = open(tarfolder_path,O_WRONLY | O_CREAT,0644);
		
		//store address of pointer to tar_totsize, no_files
		write(tar_fd,&tar_totsize,sizeof(unsigned long long));
		write(tar_fd,&no_files,sizeof(int));
		//int ct=0;
        while ((entries = readdir(dirctry))){
			if(!(entries->d_type==DT_REG)); continue;
			if (!strcmp( entries->d_name, argv[3])) continue;
			no_files++;
			char file_path[100];
			sprintf(file_path,"%s/%s",argv[2],entries->d_name);
			int files_fd= open(file_path, O_RDONLY);
            char file_name[20]; // max length 16+extra
            stpcpy(file_name, entries->d_name);
            write(tar_fd,file_name,20);//write file name 
            //write( tar_fd, " ", 1);
			filesize=(long long) size.st_size;
			tar_totsize+=filesize;
			write(files_fd,&filesize,sizeof(long long int));
			char* file_buffer= (char*) malloc(1e9); // for each file.
			//read and write filesize length chunk of data in tar file
			read(files_fd,file_buffer,filesize);
			write(tar_fd,file_buffer,filesize);
			close(files_fd); // close the file
			free(file_buffer);
        }
		lseek(tar_fd,0,SEEK_SET); // come back to top and update the total size and no_files
		write(tar_fd,&tar_totsize,sizeof(unsigned long long));
		write(tar_fd,&no_files,sizeof(int));
        closedir(dirctry);
		close(tar_fd);
    
}

void extract_tar(int argc, char *argv[]){
	if(argc!=3) {
	    printf("Failed to complete extraction operation");
	    exit(-1);
	}
	int tarfile = open(argv[2],O_RDONLY);
	//remove .tar from file name
	int i=strlen(argv[2])-1;
	while(1){
		if(argv[2][i]=='.'){
			argv[2][i] = '\0';
			break;
		}
		argv[2][i] = '\0'; //.tar becomes '\0\0\0\0'
		i--;
	} 
	char folder_name[20];

	sprintf(folder_name,"%sDump",argv[2]);
	mkdir(folder_name,0777);
	chdir(folder_name);
	char filename[20];
	int file_fd;
	int no_files=0;
	unsigned long long totsize=0,filesize=0;
	int readcontents;
	readcontents=read(tarfile,&totsize,sizeof(unsigned long long));
	readcontents=read(tarfile,&no_files,sizeof(int));
	readcontents=read(tarfile,filename,20);//wrote 20 read 20
	while(readcontents!=0){
		file_fd=open(filename,O_RDWR | O_CREAT,0644);//create file with name filename
		readcontents=read(tarfile,&filesize,sizeof(unsigned long long));
		char* file_buffer=malloc(filesize);
		read(tarfile,file_buffer,filesize); // read file content from tar
		write(file_fd,file_buffer,filesize); // write file content in file
		free(file_buffer);
		close(file_fd);
		readcontents=read(tarfile,filename,20); //read next file name if not present then become 0
	}
	close(tarfile);

}


//void single_extract_tar(int argc, char *argv[]);
//void listall_tar(int argc, char *argv[]);


int main(int argc, char *argv[])
{
	if(!(strcmp(argv[1],"-c"))) create_tar(argc,argv);
	else if (!(strcmp(argv[1],"-d"))) extract_tar(argc,argv);
	//else if (!(strcmp(argv[1],"-e"))) single_extract_tar(argc,argv);
	//else if (!(strcmp(argv[1],"-l"))) listall_tar(argc,argv);
	return 0;
}




