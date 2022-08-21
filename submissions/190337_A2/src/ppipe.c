#include<ppipe.h>
#include<context.h>
#include<memory.h>
#include<lib.h>
#include<entry.h>
#include<file.h>


// Per process information for the ppipe.
struct ppipe_info_per_process {

    // TODO:: Add members as per your need...
    //int ID; // process_number to keep oredering of all processes, but not useful here
    int read_status; // 0 = not allowed, 1 = allowed, 
    int write_status; // 0 = not allowed, 1 = allowed,
    int PID; // process_PID  req?
    int active_status; // 0 = not active, 1 = active
    int proc_read_pos; // individual read pos
    int proc_read_space; // to wrok with read space individually
    int proc_total_bytes_read; // so every flush reduce by flushed bytes, next time flush minimum of this of all active process.

};

// Global information for the ppipe.
struct ppipe_info_global {

    char *ppipe_buff;       // Persistent pipe buffer: DO NOT MODIFY THIS.

    // TODO:: Add members as per your need...
    //int read_pos; // Read position in memory 
    int global_write_pos; // Write position globally in buffer memory 
    int global_write_freespace; // free space left in buffer memory
    int process_count; // contains number of process currently connected to the pipe
    // int read_pos_min; // it will store to which pos we can flush
    // int read_pos_min_prv; // it will store current pos from where we can flush


};

// Persistent pipe structure.
// NOTE: DO NOT MODIFY THIS STRUCTURE.
struct ppipe_info {

    struct ppipe_info_per_process ppipe_per_proc [MAX_PPIPE_PROC];
    struct ppipe_info_global ppipe_global;

};


// Function to allocate space for the ppipe and initialize its members.
struct ppipe_info* alloc_ppipe_info() {

    // Allocate space for ppipe structure and ppipe buffer.
    struct ppipe_info *ppipe = (struct ppipe_info*)os_page_alloc(OS_DS_REG);
    char* buffer = (char*) os_page_alloc(OS_DS_REG);

    // Assign ppipe buffer.
    ppipe->ppipe_global.ppipe_buff = buffer;

    /**
     *  TODO:: Initializing pipe fields
     *
     *  Initialize per process fields for this ppipe.
     *  Initialize global fields for this ppipe.
     *
     */ 
    // global info
    
    ppipe->ppipe_global.global_write_pos=0;// at beginning of buffer
    ppipe->ppipe_global.global_write_freespace= MAX_PPIPE_SIZE; // full space available
    ppipe->ppipe_global.process_count=0; // defined
    // ppipe->ppipe_global.read_pos_min=0; // this update with every read
    // ppipe->ppipe_global.read_pos_min_prv=0; // only update after flush

    //per process info por process=1 //ID =0
    ppipe->ppipe_per_proc[0].read_status=1; // read allowed
    ppipe->ppipe_per_proc[0].write_status=1; // write allowed
    ppipe->ppipe_per_proc[0].active_status=1; // active
    ppipe->ppipe_per_proc[0].proc_read_pos=0; // at 0
    ppipe->ppipe_per_proc[0].proc_read_space=0;//nothing to read
    ppipe->ppipe_per_proc[0].proc_total_bytes_read=0;
    //pid fillup
    struct exec_context *current = get_current_ctx();
    ppipe->ppipe_per_proc[0].PID= current->pid;

    ppipe->ppipe_global.process_count ++; // process_count increase by 1

    // define active status of others 0
    for (int i=1; i<MAX_PPIPE_PROC;i++){
        ppipe->ppipe_per_proc[i].active_status = 0; //not active
        //ppipe->ppipe_per_proc[i].proc_total_bytes_read=0;
    }
    // Return the ppipe.
    return ppipe;

}

// Function to free ppipe buffer and ppipe info object.
// NOTE: DO NOT MODIFY THIS FUNCTION.
void free_ppipe (struct file *filep) {

    os_page_free(OS_DS_REG, filep->ppipe->ppipe_global.ppipe_buff);
    os_page_free(OS_DS_REG, filep->ppipe);

} 

// Fork handler for ppipe.
int do_ppipe_fork (struct exec_context *child, struct file *filep) {
    
    /**
     *  TODO:: Implementation for fork handler
     *
     *  You may need to update some per process or global info for the ppipe.
     *  This handler will be called twice since ppipe has 2 file objects.
     *  Also consider the limit on no of processes a ppipe can have.
     *  Return 0 on success.
     *  Incase of any error return -EOTHERS.
     *
     */
    int pid_active=0; 
    int proc_pos=-1; // if remains -1 then error
    // to check first time called or 2nd; if not 1st then found pid and update proc_pos,
    // else for 1st call assign min free per proc with active status 0; 
    for(int i=0;i<MAX_PPIPE_PROC;i++){
        if(filep->ppipe->ppipe_per_proc[i].active_status == 1){
            if(child->pid == filep->ppipe->ppipe_per_proc[i].PID) {// assigned one
                pid_active=1;
                proc_pos=i;
                break;
            }
        }
    }
    if(pid_active != 1){  // assign a pie_per_proc i
        if(filep->ppipe->ppipe_global.process_count == MAX_PPIPE_PROC) return -EOTHERS;
        for(int i=0;i<MAX_PPIPE_PROC;i++){
            if(filep->ppipe->ppipe_per_proc[i].active_status == 0){ // assign this one
                proc_pos = i;
                //filep->pipe->pipe_per_proc[proc_pos].ID = i;// ID 
                filep->ppipe->ppipe_per_proc[proc_pos].PID = child->pid; // give the pid
                filep->ppipe->ppipe_per_proc[proc_pos].active_status = 1; // make this active
                filep->ppipe->ppipe_per_proc[proc_pos].read_status = 0; // assign to 0, later change as per parent
                filep->ppipe->ppipe_per_proc[proc_pos].write_status = 0; // assign to 0, later change as per parent
                filep->ppipe->ppipe_per_proc[proc_pos].proc_total_bytes_read = 0; // assign to 0, later change as per parent
                filep->ppipe->ppipe_per_proc[proc_pos].proc_read_pos = 0; // if read closed not utilised in the read_min calculation
                filep->ppipe->ppipe_per_proc[proc_pos].proc_read_space = 0; // if read closed not utilised to read anytime
                filep->ppipe->ppipe_global.process_count +=1; // inc count by one
                break;
            }
        }
    }

    if(proc_pos == -1) return -EOTHERS;
    // allow mode_status
    // using  parent pid get it's pos
    int pid_foundp=0;
    int proc_parent_pos;
    //find process number in per_process_info
    for(int i=0;i<MAX_PPIPE_PROC;i++){
        if(filep->ppipe->ppipe_per_proc[i].active_status == 1){ // if ith is active
            if(filep->ppipe->ppipe_per_proc[i].PID == child->ppid){ // check if this is the process calling pipe_write
                pid_foundp = 1;
                proc_parent_pos = i;
                break;
            }
        }
    }
    if(pid_foundp == 0) return -EOTHERS; // current process parent is not connected to pipe

    if(filep->mode == O_READ){
        // initialize same as parent
        filep->ppipe->ppipe_per_proc[proc_pos].read_status = filep->ppipe->ppipe_per_proc[proc_parent_pos].read_status;
        filep->ppipe->ppipe_per_proc[proc_pos].proc_read_pos = filep->ppipe->ppipe_per_proc[proc_parent_pos].proc_read_pos;
        filep->ppipe->ppipe_per_proc[proc_pos].proc_read_space = filep->ppipe->ppipe_per_proc[proc_parent_pos].proc_read_space;
        filep->ppipe->ppipe_per_proc[proc_pos].proc_total_bytes_read = filep->ppipe->ppipe_per_proc[proc_parent_pos].proc_total_bytes_read;
        
    }
    else if(filep->mode == O_WRITE) filep->ppipe->ppipe_per_proc[proc_pos].write_status = filep->ppipe->ppipe_per_proc[proc_parent_pos].write_status;
    else return -EOTHERS;
    // Return successfully.
    return 0;

}


// Function to close the ppipe ends and free the ppipe when necessary.
long ppipe_close (struct file *filep) {

    /**
     *  TODO:: Implementation of Pipe Close
     *
     *  Close the read or write end of the ppipe depending upon the file
     *      object's mode.
     *  You may need to update some per process or global info for the ppipe.
     *  Use free_pipe() function to free ppipe buffer and ppipe object,
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
    for(int i=0;i<MAX_PPIPE_PROC;i++){
        if(filep->ppipe->ppipe_per_proc[i].active_status == 1){ // if ith is active
            if(filep->ppipe->ppipe_per_proc[i].PID == current->pid){ // check if this is the process calling pipe_read
                pid_found = 1;
                process_pos = i;
                break;
            }
        }
    }
    if(pid_found == 0) return -EOTHERS; // current process is not connected to pipe

    int mode = filep->mode;
    if(mode == O_READ){
        filep->ppipe->ppipe_per_proc[process_pos].read_status = 0; // close read_status
        if(filep->ppipe->ppipe_per_proc[process_pos].write_status == 0) { //both ends are closed
            filep->ppipe->ppipe_per_proc[process_pos].active_status = 0; // make it inactive
            filep->ppipe->ppipe_global.process_count -=1; // reduce process_count by 1
            // check if all process closed if yes free_ppipe()
            if(filep->ppipe->ppipe_global.process_count == 0){ 
                free_ppipe(filep);
            }
        }
    }
    else if(mode == O_WRITE){
        filep->ppipe->ppipe_per_proc[process_pos].write_status = 0; // close write_status
        if(filep->ppipe->ppipe_per_proc[process_pos].read_status == 0) { //both ends are closed
            filep->ppipe->ppipe_per_proc[process_pos].active_status = 0; // make it inactive
            filep->ppipe->ppipe_global.process_count -=1; // reduce process_count by 1
            // check if all process closed if yes free_ppipe()
            if(filep->ppipe->ppipe_global.process_count == 0){ 
                free_ppipe(filep);
            }
        }
    }
    else return -EOTHERS;

    int ret_value;

    // Close the file.
    ret_value = file_close (filep);         // DO NOT MODIFY THIS LINE.

    // And return.
    return ret_value;

}

// Function to perform flush operation on ppipe.
int do_flush_ppipe (struct file *filep) {

    /**
     *  TODO:: Implementation of Flush system call
     *
     *  Reclaim the region of the persistent pipe which has been read by 
     *      all the processes.
     *  Return no of reclaimed bytes.
     *  In case of any error return -EOTHERS.
     *
     */
    //  simple logic just update the global_write_freespace 
    //  by |read_pos_min_prv - read_pos_min|
    int reclaimed_bytes = 0;

    /************** wrong method can't work with all space written at once and min_reads end up at same place
     * 
    // update read_pos_min
    int left_min_prv_read_min=MAX_PPIPE_SIZE+1; // leftmost not possible
    int right_min_prv_read_min= MAX_PPIPE_SIZE+1; // leftmost not possible

    for(int i=0;i<MAX_PPIPE_PROC;i++){ // only use those which are active and have read enabled
        if(filep->ppipe->ppipe_per_proc[i].active_status == 1 && filep->ppipe->ppipe_per_proc[i].read_status == 1){
            if(filep->ppipe->ppipe_per_proc[i].proc_read_pos >= filep->ppipe->ppipe_global.read_pos_min_prv){ // in right
                if(right_min_prv_read_min > filep->ppipe->ppipe_per_proc[i].proc_read_pos){
                    right_min_prv_read_min = filep->ppipe->ppipe_per_proc[i].proc_read_pos;
                }
            }
            else {
                if(left_min_prv_read_min > filep->ppipe->ppipe_per_proc[i].proc_read_pos){
                    left_min_prv_read_min = filep->ppipe->ppipe_per_proc[i].proc_read_pos;
                }
            }
        }
    }
    // if right_min_prv_read_min remaains unchanged then all read pos have gone to left side of prv min 
    // so update with it's min val, else update with min value in right of min prev
    if(right_min_prv_read_min == MAX_PPIPE_SIZE + 1) {
        filep->ppipe->ppipe_global.read_pos_min= left_min_prv_read_min;
    }
    else filep->ppipe->ppipe_global.read_pos_min= right_min_prv_read_min; 


    if(filep->ppipe->ppipe_global.read_pos_min_prv <= filep->ppipe->ppipe_global.read_pos_min) {
        // in right , min marks not rotated
        reclaimed_bytes = filep->ppipe->ppipe_global.read_pos_min - filep->ppipe->ppipe_global.read_pos_min_prv;
    }
    else {
        // rotated, new min in left of prv min
        reclaimed_bytes = MAX_PPIPE_SIZE-(filep->ppipe->ppipe_global.read_pos_min_prv - filep->ppipe->ppipe_global.read_pos_min);
    }

    filep->ppipe->ppipe_global.global_write_freespace +=reclaimed_bytes;// reclaimed bytes added to write space
    filep->ppipe->ppipe_global.read_pos_min_prv = filep->ppipe->ppipe_global.read_pos_min; // update prv min

    *******************/

    // take minimum in read_bytes count of each active process with read end working

    int min_read_count = MAX_PPIPE_SIZE +1;
    int one_active =0;
    for(int i=0;i<MAX_PPIPE_PROC;i++){ // only use those which are active and have read enabled
        if(filep->ppipe->ppipe_per_proc[i].active_status == 1 && filep->ppipe->ppipe_per_proc[i].read_status == 1){
            one_active = 1;
            if(filep->ppipe->ppipe_per_proc[i].proc_total_bytes_read < min_read_count) {
                min_read_count = filep->ppipe->ppipe_per_proc[i].proc_total_bytes_read;
            }
        }
    }
    if(one_active == 1) reclaimed_bytes = min_read_count;   
    else reclaimed_bytes = 0; // no read end open for any process, so flsuh 0 bytes
    
    if(one_active == 1){ 
        //reduce the proc_total_bytes_read for each process by min_read_count, only doing for active to read process because others anyhow not contribute.
        for(int i=0;i<MAX_PPIPE_PROC;i++){ // only use those which are active and have read enabled
            if(filep->ppipe->ppipe_per_proc[i].active_status == 1 && filep->ppipe->ppipe_per_proc[i].read_status == 1){
                filep->ppipe->ppipe_per_proc[i].proc_total_bytes_read -= min_read_count;
            }
        }
    }

    filep->ppipe->ppipe_global.global_write_freespace +=reclaimed_bytes;// reclaimed bytes added to write space

    // Return reclaimed bytes.
    return reclaimed_bytes;

}

// Read handler for the ppipe.
int ppipe_read (struct file *filep, char *buff, u32 count) {
    
    /**
     *  TODO:: Implementation of PPipe Read
     *
     *  Read the data from ppipe buffer and write to the provided buffer.
     *  If count is greater than the present data size in the ppipe then just read
     *      that much data.
     *  Validate file object's access right.
     *  On successful read, return no of bytes read.
     *  Incase of Error return valid error code.
     *      -EACCES: In case access is not valid.
     *      -EINVAL: If read end is already closed.
     *      -EOTHERS: For any other errors.
     *
     */
    // while we read from pipe buffer, we write it to the given buffer
    // so need to check for write permission for it

    // get the ID in per_process_info
    struct exec_context * current = get_current_ctx();
    int pid_found = 0; // false
    //find process number in per_process_info
    int process_pos;
    for(int i=0;i<MAX_PPIPE_PROC;i++){
        if(filep->ppipe->ppipe_per_proc[i].active_status == 1){ // if ith is active
            if(filep->ppipe->ppipe_per_proc[i].PID == current->pid){ // check if this is the process calling pipe_read
                pid_found = 1;
                process_pos = i;
                break;
            }
        }
    }
    
    if(pid_found == 0) return -EOTHERS; // current process is not connected to pipe

    // check for read end access 
    if(filep->ppipe->ppipe_per_proc[process_pos].read_status !=1) return -EINVAL;

    //read from cicular queue logic
    int bytes_read = 0;
    if(count > filep->ppipe->ppipe_per_proc[process_pos].proc_read_space ) {
        bytes_read = filep->ppipe->ppipe_per_proc[process_pos].proc_read_space;
    }
    else bytes_read =count;
    // read from ppipe bufer
    for(int i=0;i<bytes_read;i++){
        int ppipe_buff_pos= (filep->ppipe->ppipe_per_proc[process_pos].proc_read_pos) % MAX_PPIPE_SIZE;
        buff[i]= filep->ppipe->ppipe_global.ppipe_buff[ppipe_buff_pos];
        filep->ppipe->ppipe_per_proc[process_pos].proc_read_pos = (ppipe_buff_pos+1) % MAX_PPIPE_SIZE;
    }
    filep->ppipe->ppipe_per_proc[process_pos].proc_read_space -=bytes_read; // reduce free space by read bytes for that process
    filep->ppipe->ppipe_per_proc[process_pos].proc_total_bytes_read +=bytes_read; // increase the read bytes count for process
    // Return no of bytes read.
    return bytes_read;
	
}

// Write handler for ppipe.
int ppipe_write (struct file *filep, char *buff, u32 count) { // where error?

    /**
     *  TODO:: Implementation of PPipe Write
     *
     *  Write the data from the provided buffer to the ppipe buffer.
     *  If count is greater than available space in the ppipe then just write
     *      data that fits in that space.
     *  Validate file object's access right.
     *  On successful write, return no of written bytes.
     *  Incase of Error return valid error code.
     *      -EACCES: In case access is not valid.
     *      -EINVAL: If write end is already closed.
     *      -EOTHERS: For any other errors.
     *
     */

    // while we write from pipe buffer, we read the data from the given buffer
    // so need to check for read permission for it
    //if(is_valid_mem_range(buff,count,1)!= 1) return EACCES;

    // get the ID in per_process_info
    struct exec_context * current = get_current_ctx();
    int pid_found = 0; // false
    //find process number in per_process_info
    int process_pos;
    for(int i=0;i<MAX_PPIPE_PROC;i++){
        if(filep->ppipe->ppipe_per_proc[i].active_status == 1){ // if ith is active
            if(filep->ppipe->ppipe_per_proc[i].PID == current->pid){ // check if this is the process calling pipe_write
                pid_found = 1;
                process_pos = i;
                break;
            }
        }
    }
    if(pid_found == 0) return -EOTHERS; // current process is not connected to pipe
    
    // check for write end access 
    if(filep->ppipe->ppipe_per_proc[process_pos].write_status !=1) return -EINVAL;

    //write from cicular queue logic
    int bytes_written = 0;

    if(count > filep->ppipe->ppipe_global.global_write_freespace ) {
        bytes_written = filep->ppipe->ppipe_global.global_write_freespace; // full freespace can be filled
    }
    else bytes_written=count;
    // write from buffer
    for(int i=0;i<bytes_written;i++) {
        int ppipe_buff_pos= (filep->ppipe->ppipe_global.global_write_pos) % MAX_PPIPE_SIZE;
        filep->ppipe->ppipe_global.ppipe_buff[ppipe_buff_pos]=buff[i];
        filep->ppipe->ppipe_global.global_write_pos= (ppipe_buff_pos+1) % MAX_PPIPE_SIZE;
    }
    filep->ppipe->ppipe_global.global_write_freespace -=bytes_written; // decrease freespace by written bytes

    // update proc_read_space for each active process by bytes_written
    for(int i=0;i<MAX_PPIPE_PROC;i++){
        if(filep->ppipe->ppipe_per_proc[i].active_status == 1){
            filep->ppipe->ppipe_per_proc[i].proc_read_space+=bytes_written;
        }
    }

    // Return no of bytes written.
    return bytes_written;

}

// Function to create persistent pipe.
int create_persistent_pipe (struct exec_context *current, int *fd) {

    /**
     *  TODO:: Implementation of PPipe Create
     *
     *  Find two free file descriptors.
     *  Create two file objects for both ends by invoking the alloc_file() function.
     *  Create ppipe_info object by invoking the alloc_ppipe_info() function and
     *      fill per process and global info fields.
     *  Fill the fields for those file objects like type, fops, etc.
     *  Fill the valid file descriptor in *fd param.
     *  On success, return 0.
     *  Incase of Error return valid Error code.
     *      -ENOMEM: If memory is not enough.
     *      -EOTHERS: Some other errors.
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
    struct ppipe_info *ppipe_og = alloc_ppipe_info();
    // fill file object details
    fo1->ppipe = ppipe_og;
    fo2->ppipe = ppipe_og;
    fo1->mode = O_READ; // file object 1 for reading
    fo2->mode = O_WRITE; // file object 2 for writing
    fo1->type = PPIPE; // setting it's type PPIPE
    fo2->type = PPIPE;

    // fo1->inode = ?
    // fo2->inode = ?
    // are these even needed? -- not required to fill these.
    
    // assign callbacks how? filep=? buffer=? count=?--use function pointer directly
    fo1->fops->read = &ppipe_read;
    fo1->fops->write = &ppipe_write;
    fo1->fops->close = &ppipe_close;

    fo2->fops->read = &ppipe_read;
    fo2->fops->write = &ppipe_write;
    fo2->fops->close = &ppipe_close;

    // fill the current with fo1 & fo2
    current->files[fd1_pos]=fo1; // read
    current->files[fd2_pos]=fo2; // write

    // Fill the valid file descriptor in *fd param. 
    fd[0] = fd1_pos ;// to read
    fd[1] = fd2_pos ;// to write

    // Simple return.
    return 0;

}
