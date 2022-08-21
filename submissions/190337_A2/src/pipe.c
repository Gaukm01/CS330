#include<pipe.h>
#include<context.h>
#include<memory.h>
#include<lib.h>
#include<entry.h>
#include<file.h>


// Per process info for the pipe.
struct pipe_info_per_process {

    // TODO:: Add members as per your need...
    //int ID; // process_number to keep oredering of all processes, but not useful here
    int read_status; // 0 = not allowed, 1 = allowed, 
    int write_status; // 0 = not allowed, 1 = allowed,
    int PID; // process_PID  req?
    int active_status; // 0 = not active, 1 = active

};

// Global information for the pipe.
struct pipe_info_global {

    char *pipe_buff;    // Pipe buffer: DO NOT MODIFY THIS.
    // TODO:: Add members as per your need...

    int read_pos; // Read position in memory 
    int write_pos; // Write position in buffer memory 
    int freespace; // free space left in buffer memory
    int process_count; // contains number of process currently connected to the pipe

};

// Pipe information structure.
// NOTE: DO NOT MODIFY THIS STRUCTURE.
struct pipe_info {

    struct pipe_info_per_process pipe_per_proc [MAX_PIPE_PROC];
    struct pipe_info_global pipe_global;

};


// Function to allocate space for the pipe and initialize its members.
struct pipe_info* alloc_pipe_info () {
	
    // Allocate space for pipe structure and pipe buffer.
    struct pipe_info *pipe = (struct pipe_info*)os_page_alloc(OS_DS_REG);
    char* buffer = (char*) os_page_alloc(OS_DS_REG);
    
    // Assign pipe buffer.
    pipe->pipe_global.pipe_buff = buffer;

    /**
     *  TODO:: Initializing pipe fields
     *  
     *  Initialize per process fields for this pipe.
     *  Initialize global fields for this pipe.
     *  
     *
     */
    // global info
    pipe->pipe_global.read_pos=0; // at the beginning of buffer
    pipe->pipe_global.write_pos=0;// at beginning of buffer
    pipe->pipe_global.freespace=MAX_PIPE_SIZE; // full space available
    pipe->pipe_global.process_count=0; // defined
    //per process info por process=1 //ID =0
    //pipe->pipe_per_proc[0].ID=pipe->pipe_global.process_count; // ID= 0 given.
    pipe->pipe_per_proc[0].read_status=1; // read allowed
    pipe->pipe_per_proc[0].write_status=1; // write allowed
    pipe->pipe_per_proc[0].active_status=1; // active
    //pid fillup
    struct exec_context *current = get_current_ctx();
    pipe->pipe_per_proc[0].PID= current->pid;

    pipe->pipe_global.process_count ++; // process_count increase by 1

    // define active status of others 0
    for (int i=1; i<MAX_PIPE_PROC;i++){
        pipe->pipe_per_proc[i].active_status = 0; //not active
    }
    // Return the pipe.
    return pipe;

}

// Function to free pipe buffer and pipe info object.
// NOTE: DO NOT MODIFY THIS FUNCTION.
void free_pipe (struct file *filep) {

    os_page_free(OS_DS_REG, filep->pipe->pipe_global.pipe_buff);
    os_page_free(OS_DS_REG, filep->pipe);

}

// Fork handler for the pipe.
int do_pipe_fork (struct exec_context *child, struct file *filep) {

    /**
     *  TODO:: Implementation for fork handler
     *
     *  You may need to update some per process or global info for the pipe.
     *  This handler will be called twice since pipe has 2 file objects.
     *  Also consider the limit on no of processes a pipe can have.
     *  Return 0 on success.
     *  Incase of any error return -EOTHERS.
     *
     */
    

    int pid_active=0; 
    int proc_pos;
    // to check first time called or 2nd; if not 1st then found pid and update proc_pos,
    // else for 1st call assign min free per proc with active status 0; 
    for(int i=0;i<MAX_PIPE_PROC;i++){
        if(filep->pipe->pipe_per_proc[i].active_status == 1){
            if(child->pid == filep->pipe->pipe_per_proc[i].PID) {// assigned one
                pid_active=1;
                proc_pos=i;
                break;
            }
        }
    }
    if(pid_active != 1){  // assign a pie_per_proc i
        if(filep->pipe->pipe_global.process_count == MAX_PIPE_PROC) return -EOTHERS;
        for(int i=0;i<MAX_PIPE_PROC;i++){
            if(filep->pipe->pipe_per_proc[i].active_status == 0){ // assign this one
                proc_pos = i;
                //filep->pipe->pipe_per_proc[proc_pos].ID = i;// ID 
                filep->pipe->pipe_per_proc[proc_pos].PID = child->pid; // give the pid
                filep->pipe->pipe_per_proc[proc_pos].active_status = 1; // make this active
                filep->pipe->pipe_per_proc[proc_pos].read_status = 0; // initialize, later makesame as parent
                filep->pipe->pipe_per_proc[proc_pos].write_status = 0;// initialize, later makesame as parent
                filep->pipe->pipe_global.process_count +=1; // inc count by one
                break;
            }
        }
    }
    // allow mode_status
    // using  parent pid get it's pos
    int pid_foundp=0;
    int proc_parent_pos;
    //find process number in per_process_info
    for(int i=0;i<MAX_PIPE_PROC;i++){
        if(filep->pipe->pipe_per_proc[i].active_status == 1){ // if ith is active
            if(filep->pipe->pipe_per_proc[i].PID == child->ppid){ // check if this is the process calling pipe_write
                pid_foundp = 1;
                proc_parent_pos = i;
                break;
            }
        }
    }
    if(pid_foundp == 0) return -EOTHERS; // current process is not connected to pipe

    if(filep->mode == O_READ) filep->pipe->pipe_per_proc[proc_pos].read_status = filep->pipe->pipe_per_proc[proc_parent_pos].read_status;
    else if(filep->mode == O_WRITE) filep->pipe->pipe_per_proc[proc_pos].write_status = filep->pipe->pipe_per_proc[proc_parent_pos].write_status;
    else return -EOTHERS;


    // Return successfully.
    return 0;

}

// Function to close the pipe ends and free the pipe when necessary.
long pipe_close (struct file *filep) {

    /**
     *  TODO:: Implementation of Pipe Close
     *
     *  Close the read or write end of the pipe depending upon the file
     *      object's mode.
     *  You may need to update some per process or global info for the pipe.
     *  Use free_pipe() function to free pipe buffer and pipe object,
     *      whenever applicable.
     *  After successful close, it return 0.
     *  Incase of any error return -EOTHERS.
     *
     */

    // get the ID in per_process_info
    struct exec_context * current = get_current_ctx();
    int pid_found = 0; // false
    int process_pos;
    //find process number in per_process_info
    for(int i=0;i<MAX_PIPE_PROC;i++){
        if(filep->pipe->pipe_per_proc[i].active_status == 1){ // if ith is active
            if(filep->pipe->pipe_per_proc[i].PID == current->pid){ // check if this is the process calling pipe_read
                pid_found = 1;
                process_pos = i;
                break;
            }
        }
    }
    if(pid_found == 0) return -EOTHERS; // current process is not connected to pipe


    // first check about what kind of mode filep have,
    // then accordingly update read/write_status
    // check if both ends read and write is closed then reduce process_count by 1
    // if process_count becomes 0 free pipe buffer.

    int mode = filep->mode;
    if(mode == O_READ){
        filep->pipe->pipe_per_proc[process_pos].read_status = 0; // close read_status
        if(filep->pipe->pipe_per_proc[process_pos].write_status == 0) { //both ends are closed
            filep->pipe->pipe_per_proc[process_pos].active_status = 0; // make it inactive
            filep->pipe->pipe_global.process_count -=1; // reduce process_count by 1
            // check if all process closed if yes free_pipe()
            if(filep->pipe->pipe_global.process_count == 0){ 
                free_pipe(filep);
            }
        }
    }
    else if(mode == O_WRITE){
        filep->pipe->pipe_per_proc[process_pos].write_status = 0; // close write_status
        if(filep->pipe->pipe_per_proc[process_pos].read_status == 0) { //both ends are closed
            filep->pipe->pipe_per_proc[process_pos].active_status = 0; // make it inactive
            filep->pipe->pipe_global.process_count -=1; // reduce process_count by 1
            // check if all process closed if yes free_pipe()
            if(filep->pipe->pipe_global.process_count == 0){ 
                free_pipe(filep);
            }
        }
    }
    else return -EOTHERS;

    //filep = NULL; maybe already done by defined func
    int ret_value;

    // Close the file and return.
    ret_value = file_close (filep);         // DO NOT MODIFY THIS LINE.

    // And return.
    return ret_value;

}

// Check whether passed buffer is valid memory location for read or write.
int is_valid_mem_range (unsigned long buff, u32 count, int access_bit) {

    /**
     *  TODO:: Implementation for buffer memory range checking
     *
     *  Check whether passed memory range is suitable for read or write.
     *  If access_bit == 1, then it is asking to check read permission.
     *  If access_bit == 2, then it is asking to check write permission.
     *  If range is valid then return 1.
     *  Incase range is not valid or have some permission issue return -EBADMEM.
     *
     */
    
    int ret_value = -EBADMEM;

    // get the current process details 
    struct exec_context * current = get_current_ctx();
     
    // find position of buffer in the memory and check access permissions
    for(int i=0; i<MAX_MM_SEGS; i++) {// total 5 modes
        // if stack whole start to end works
        if(i == MM_SEG_STACK) {
            if(buff >= current->mms[i].start && buff <= current->mms[i].end && buff+count <= current->mms[i].end) {
                if((current->mms[i].access_flags & access_bit) !=0){ // not equal to 0 that means acces is allowed
                    ret_value =1;
                    return ret_value;
                }
                else { // else found but no permission to access.
                    ret_value = -EBADMEM;
                    return ret_value;
                }
            }
        }
        else { // not stack  // check based on next_free
            if(buff >= current->mms[i].start && buff <= current->mms[i].next_free && buff+count <= current->mms[i].next_free) {
                if((current->mms[i].access_flags & access_bit) !=0){ // not equal to 0 that means access is allowed
                    ret_value =1;
                    return ret_value;
                }
                else { // else found but no permission to access.
                    ret_value = -EBADMEM; 
                    return ret_value;
                }
            }     
        }
    }
    // now check in vm_memory
    struct vm_area * temp;
    temp = current->vm_area;
    while (temp != NULL){
        if(buff >= temp->vm_start && buff <= temp->vm_end && buff+count <= temp->vm_end) {
            if((temp->access_flags & access_bit) != 0 ) { // access allowed
                ret_value =1;
                return ret_value;
            }
            else { // found memory but permission to access fails
                ret_value= -EBADMEM;
                return ret_value;
            }
        }
        temp= temp->vm_next; // ye line rh gya final submission me
    }

    // never found in memory segment then return -EBADMEM or found that buffer lies in some space 
    // but buffer+count does not lies in that, then eventually we reach here to give error.
    // Return the finding.
    return ret_value;

}

// Function to read given no of bytes from the pipe.
int pipe_read (struct file *filep, char *buff, u32 count) {

    /**
     *  TODO:: Implementation of Pipe Read
     *
     *  Read the data from pipe buffer and write to the provided buffer.
     *  If count is greater than the present data size in the pipe then just read
     *       that much data.
     *  Validate file object's access right.
     *  On successful read, return no of bytes read.
     *  Incase of Error return valid error code.
     *       -EACCES: In case access is not valid.
     *       -EINVAL: If read end is already closed.
     *       -EOTHERS: For any other errors.
     *
     */

    // while we read from pipe buffer, we write it to the given buffer
    // so need to check for write permission for it
    if(is_valid_mem_range((unsigned long) buff,count,2)!= 1) return -EACCES;

    // get the ID in per_process_info
    struct exec_context * current = get_current_ctx();
    int pid_found = 0; // false
    //find process number in per_process_info
    int process_pos;
    for(int i=0;i<MAX_PIPE_PROC;i++){
        if(filep->pipe->pipe_per_proc[i].active_status == 1){ // if ith is active
            if(filep->pipe->pipe_per_proc[i].PID == current->pid){ // check if this is the process calling pipe_read
                pid_found = 1;
                process_pos = i;
                break;
            }
        }
    }
    
    if(pid_found == 0) return -EOTHERS; // current process is not connected to pipe

    // check for read end access 
    if(filep->pipe->pipe_per_proc[process_pos].read_status !=1) return -EINVAL;

    //read from cicular queue logic
    int bytes_read = 0;
    if(count > MAX_PIPE_SIZE- filep->pipe->pipe_global.freespace ) {
        bytes_read = MAX_PIPE_SIZE - filep->pipe->pipe_global.freespace;
    }
    else bytes_read =count;
    for(int i=0;i<bytes_read;i++){
        int pipe_buff_pos= (filep->pipe->pipe_global.read_pos) % MAX_PIPE_SIZE;
        buff[i]= filep->pipe->pipe_global.pipe_buff[pipe_buff_pos];
        filep->pipe->pipe_global.read_pos= (pipe_buff_pos+1) % MAX_PIPE_SIZE;
    }
    filep->pipe->pipe_global.freespace+=bytes_read; // increase free space by read bytes
    // Return no of bytes read.
    return bytes_read;

}

// Function to write given no of bytes to the pipe.
int pipe_write (struct file *filep, char *buff, u32 count) {

    /**
     *  TODO:: Implementation of Pipe Write
     *
     *  Write the data from the provided buffer to the pipe buffer.
     *  If count is greater than available space in the pipe then just write data
     *       that fits in that space.
     *  Validate file object's access right.
     *  On successful write, return no of written bytes.
     *  Incase of Error return valid error code.
     *       -EACCES: In case access is not valid.
     *       -EINVAL: If write end is already closed.
     *       -EOTHERS: For any other errors.
     *
     */

    // while we write from pipe buffer, we read the data from the given buffer
    // so need to check for read permission for it
    if(is_valid_mem_range((unsigned long) buff,count,1)!= 1) return -EACCES;

    // get the ID in per_process_info
    struct exec_context * current = get_current_ctx();
    int pid_found = 0; // false
    //find process number in per_process_info
    int process_pos;
    for(int i=0;i<MAX_PIPE_PROC;i++){
        if(filep->pipe->pipe_per_proc[i].active_status == 1){ // if ith is active
            if(filep->pipe->pipe_per_proc[i].PID == current->pid){ // check if this is the process calling pipe_write
                pid_found = 1;
                process_pos = i;
                break;
            }
        }
    }
    if(pid_found == 0) return -EOTHERS; // current process is not connected to pipe
    
    // check for wrtie end access 
    if(filep->pipe->pipe_per_proc[process_pos].write_status !=1) return -EINVAL;

    //write from cicular queue logic
    int bytes_written = 0;

    if(count > filep->pipe->pipe_global.freespace ) {
        bytes_written = filep->pipe->pipe_global.freespace; // full freespace can be filled
    }
    else bytes_written=count;

    for(int i=0;i<bytes_written;i++) {
        int pipe_buff_pos= (filep->pipe->pipe_global.write_pos) % MAX_PIPE_SIZE;
        filep->pipe->pipe_global.pipe_buff[pipe_buff_pos]=buff[i];
        filep->pipe->pipe_global.write_pos= (pipe_buff_pos+1) % MAX_PIPE_SIZE;
    }
    filep->pipe->pipe_global.freespace -=bytes_written; // reduce free space by written bytes

    // Return no of bytes written.
    return bytes_written;

}

// Function to create pipe.
int create_pipe (struct exec_context *current, int *fd) {

    /**
     *  TODO:: Implementation of Pipe Create
     *
     *  Find two free file descriptors. ---?
     *  Create two file objects for both ends by invoking the alloc_file() function. 
     *  Create pipe_info object by invoking the alloc_pipe_info() function and
     *       fill per process and global info fields.
     *  Fill the fields for those file objects like type, fops, etc.
     *  Fill the valid file descriptor in *fd param.
     *  On success, return 0.
     *  Incase of Error return valid Error code.
     *       -ENOMEM: If memory is not enough.
     *       -EOTHERS: Some other errors.
     *
     */

    //Find two free file descriptors.
    int fd1_pos=-1,fd2_pos=-1; //two smallest free file descritpors pos
    //check for free file descriptor pos 
    for (int i=0; i<MAX_OPEN_FILES; i++){
        if (current->files[i] ==NULL){
            if(fd1_pos==-1) fd1_pos= i;
            else if(fd2_pos==-1) {fd2_pos=i; break;}
        }
    }
    if (fd1_pos==-1 || fd2_pos==-1) return -EOTHERS;

    //create file objects
    struct file *fo1 = alloc_file(); // file object 1
    struct file *fo2 = alloc_file(); // file object 2
    // create pipe
    struct pipe_info *pipe_og = alloc_pipe_info();
    // fill file object details
    fo1->pipe = pipe_og;
    fo2->pipe = pipe_og;
    fo1->mode = O_READ; // file object 1 for reading
    fo2->mode = O_WRITE; // file object 2 for writing

    // fo1->inode = ?
    // fo2->inode = ?
    fo1->type = PIPE;
    fo2->type = PIPE;
    // are these even needed? -- not required to fill these.
    
    // assign callbacks how? filep=? buffer=? count=?--use function pointer directly
    fo1->fops->read = &pipe_read;
    fo1->fops->write = &pipe_write;
    fo1->fops->close = &pipe_close;

    fo2->fops->read = &pipe_read;
    fo2->fops->write = &pipe_write;
    fo2->fops->close = &pipe_close;

    // fill the current with fo1 & fo2
    current->files[fd1_pos]=fo1; // read
    current->files[fd2_pos]=fo2; // write

    // Fill the valid file descriptor in *fd param. 
    fd[0] = fd1_pos ;// to read
    fd[1] = fd2_pos ;// to write


    // Simple return.
    return 0;

}
