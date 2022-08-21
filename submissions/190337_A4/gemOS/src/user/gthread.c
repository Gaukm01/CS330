#include <gthread.h>
#include <ulib.h>

static struct process_thread_info tinfo __attribute__((section(".user_data"))) = {};
/*XXX 
      Do not modifiy anything above this line. The global variable tinfo maintains user
      level accounting of threads. Refer gthread.h for definition of related structs.
 */


// gthred alterante exit function to handle case gthread_exit() is not called by th_func
int gthread_alternate_exit(){
	void* ret;
	asm volatile(
	"mov %%rax, %0;"
	: "=r" (ret)
	:
	:
	);
	gthread_exit(ret); // calls gthread_exit with return value earlier stored in
	return 0;
}

/* Returns 0 on success and -1 on failure */
/* Here you can use helper system call "make_thread_ready" for your implementation */
int gthread_create(int *tid, void *(*fc)(void *), void *arg) {
        
	/* You need to fill in your implementation here*/
	if(tinfo.num_threads >= MAX_THREADS) return -1;

	int i=0, found =0;
	struct thread_info* th;
	for(i =0; i< MAX_THREADS; i++){
		th = &tinfo.threads[i];
		if(th->status != TH_STATUS_ALIVE){ // && th->status != TH_STATUS_USED){
			found = 1;
			//printf("REACHED HERE found a Tinfo \n");
			void* th_stack = mmap(NULL, TH_STACK_SIZE, PROT_READ|PROT_WRITE, 0);
			
			// push a function in stack so that if gthread_exit() is not called than
			// this function get called and call exit(0)
			*(u64 *)((u64)th_stack + TH_STACK_SIZE - 8) = (u64)gthread_alternate_exit;
			int th_pid = clone(fc, ((u64)th_stack)+ TH_STACK_SIZE -8, arg );

			if(th_pid != -1){
				//printf("clone is succesful \n");
				th->status = TH_STATUS_ALIVE;
				th->tid = i ;
				th->ret_addr = arg;
				th->stack_addr = th_stack;
				*tid = i;
				th->pid = th_pid;
				th->exit_status = 0; // not exited 
				tinfo.num_threads +=1;
			}
			else {
				//printf("clone is not succesful \n");
				//os_free(th_stack);
				int ret=munmap(th_stack,TH_STACK_SIZE);
				return -1;
			}
			make_thread_ready(th_pid);

			break;
		}
		
	}
	if (found == 0) return -1;
	return 0;
}

int gthread_exit(void *retval) {

	/* You need to fill in your implementation here*/
	int th_pid = getpid(),found=0,i=0;
	struct thread_info* th;
	for(i =0; i< MAX_THREADS; i++){
		th = &tinfo.threads[i];
		if(th->pid == th_pid && th->status == TH_STATUS_ALIVE){
			found = 1;
			th->ret_addr = retval;
			th->exit_status =1 ;// exited --- execution of thread completed.
			//th->status = TH_STATUS_ALIVE;
			//if(munmap(th->stack_addr,TH_STACK_SIZE)<0) return -1;
			//exit(0);
			break;
		}
	}
	//if(found ==0) return ?;
	//call exit
	exit(0);
}

void* gthread_join(int tid) {
        
     /* Here you can use helper system call "wait_for_thread" for your implementation */
	 struct thread_info* th;
	 int found=0,i=0;
	 for(i =0; i< MAX_THREADS; i++){
		th = &tinfo.threads[i];
		if(th->tid == tid){
			found =1;
			break;
		}
	 }
	 if (!found) return NULL;
	 while(1) {
		 int finish= wait_for_thread(th->pid);
		 if ((th->exit_status==1) || finish) break;
	 }
	 th->status = TH_STATUS_USED;
	 tinfo.num_threads--;
	 void* ret_value = (void*)th->ret_addr;
	 if(munmap(th->stack_addr,TH_STACK_SIZE) <0) return (int*)-1; //MAP_ERR
	 return ret_value;
       
     /* You need to fill in your implementation here*/

}


/*Only threads will invoke this. No need to check if its a process
 * The allocation size is always < GALLOC_MAX and flags can be one
 * of the alloc flags (GALLOC_*) defined in gthread.h. Need to 
 * invoke mmap using the proper protection flags (for prot param to mmap)
 * and MAP_TH_PRIVATE as the flag param of mmap. The mmap call will be 
 * handled by handle_thread_private_map in the OS.
 * */

void* gmalloc(u32 size, u8 alloc_flag)
{
   
	/* You need to fill in your implementation here*/
	int th_pid= getpid(),i=0,j=0;
	void *priv_map_addr;
	struct thread_info* th;
	for(i =0; i< MAX_THREADS; i++){
		th = &tinfo.threads[i];
		if(th->pid == th_pid && th->status == TH_STATUS_ALIVE){
			
			for(j=0;j<MAX_GALLOC_AREAS;j++){
				if(th->priv_areas[j].length == 0){
					//virtual address with required prot permissions for different cases
					if( alloc_flag == GALLOC_OWNONLY){
						priv_map_addr = mmap(NULL, size, PROT_READ|PROT_WRITE | TP_SIBLINGS_NOACCESS, MAP_TH_PRIVATE);
					}
					else if( alloc_flag == GALLOC_OTRDONLY){
						priv_map_addr = mmap(NULL, size, PROT_READ|PROT_WRITE | TP_SIBLINGS_RDONLY, MAP_TH_PRIVATE);
					}
					else if( alloc_flag == GALLOC_OTRDWR){
						priv_map_addr = mmap(NULL, size, PROT_READ|PROT_WRITE | TP_SIBLINGS_RDWR, MAP_TH_PRIVATE);
					}
					else return NULL; // error case
					th->priv_areas[j].owner = th;
					th->priv_areas[j].start = (u64) priv_map_addr;
					th->priv_areas[j].length = size;
					th->priv_areas[j].flags = alloc_flag;
					break;
				}
			}
			break;
		}
	}	 void* ret_value = (void*)th->ret_addr;


	return priv_map_addr;


}
/*
   Only threads will invoke this. No need to check if the caller is a process.
*/
int gfree(void *ptr)
{
   
    /* You need to fill in your implementation here*/
	int th_pid= getpid(),i=0,j=0;
	struct thread_info* th;
	for(i =0; i< MAX_THREADS; i++){
		th = &tinfo.threads[i];
		if(th->pid == th_pid && th->status == TH_STATUS_ALIVE){
			u8 th_tid = th->tid;
			for(j=0;j<MAX_GALLOC_AREAS;j++){

				if(th->priv_areas[j].owner->tid == th_tid && th->priv_areas[j].start == *(u64*) ptr){
					// munmap the allocated memory
					if(munmap(ptr, th->priv_areas[j].length) <0) return -1; // munmap failed
					
					th->priv_areas[j].owner = NULL;
					th->priv_areas[j].start = -1;
					th->priv_areas[j].length = 0;
					th->priv_areas[j].flags = 0x0;

					return 0;
				}
			}
			break;
		}
	}

    return -1;
}
