#include<clone_threads.h>
#include<entry.h>
#include<context.h>
#include<memory.h>
#include<lib.h>
#include<mmap.h>

/*
  system call handler for clone, create thread like 
  execution contexts. Returns pid of the new context to the caller. 
  The new context starts execution from the 'th_func' and 
  use 'user_stack' for its stack
*/
long do_clone(void *th_func, void *user_stack, void *user_arg) 
{
  


  struct exec_context *new_ctx = get_new_ctx();
  struct exec_context *ctx = get_current_ctx();

  u32 pid = new_ctx->pid;
  
  if(!ctx->ctx_threads){  // This is the first thread
          ctx->ctx_threads = os_alloc(sizeof(struct ctx_thread_info));
          bzero((char *)ctx->ctx_threads, sizeof(struct ctx_thread_info));
          ctx->ctx_threads->pid = ctx->pid;
  }
     
 /* XXX Do not change anything above. Your implementation goes here*/
  
  
  // allocate page for os stack in kernel part of process's VAS
  // The following two lines should be there. The order can be 
  // decided depending on your logic.
  
  

  setup_child_context(new_ctx); // position of this fix accordingly.

  // first we will copy the parent process in thread and then do the required changes
  *new_ctx = *ctx;
  // now just then thread info which need to be changed.
  new_ctx->ppid = ctx->pid;
  new_ctx->pid = pid;
  new_ctx->type = EXEC_CTX_USER_TH;    // Make sure the context type is thread
  
  //new_ctx->state = UNUSED;            // For the time being. Remove it as per your need.

  // fix the register values as needed
  new_ctx->regs.entry_rip = th_func;
  new_ctx->regs.entry_rsp = user_stack;
  new_ctx->regs.rdi = user_arg;

  // make a new thread
  struct thread *new_thread = os_alloc(sizeof(struct thread));


  int found = 0, i = 0;

  for (i = 0; i < MAX_THREADS; i++) { // find unused thread then fill details after allocating
      if (ctx->ctx_threads->threads[i].status == TH_UNUSED ) {
          ctx->ctx_threads->threads[i] = *new_thread;
          ctx->ctx_threads->threads[i].parent_ctx = ctx;
          ctx->ctx_threads->threads[i].status = TH_USED;
          ctx->ctx_threads->threads[i].pid = pid; 
          //ctx->ctx_threads->threads[i].private_mappings = ?;

          found = 1;
          break;
      }
  }
  if (found) {
    new_ctx->state = WAITING; // state initilize to WAITING
    return pid;
  }

  // struct thread* th = find_unused_thread(ctx);
  // if (th == NULL) return -1;
  // else {
  //     th = new_thread;
  //     th->parent_ctx = ctx;
  //     th->status = TH_USED;
  //     th->pid = pid;
  //     // th->private_mappings =?;

  //     new_ctx->state = WAITING;
  //     return pid;
  // }

  return -1;

}

/*This is the page fault handler for thread private memory area (allocated using 
 * gmalloc from user space). This should fix the fault as per the rules. If the the 
 * access is legal, the fault handler should fix it and return 1. Otherwise it should
 * invoke segfault_exit and return -1*/

int handle_thread_private_fault(struct exec_context *current, u64 addr, int error_code)
{
  
   /* your implementation goes here*/
    int ppid = current->ppid; // called from threads
    int th_pid = current->pid;
    struct exec_context *parent_ctx = get_ctx_by_pid(ppid); 
    struct thread* th = find_thread_from_pid(parent_ctx, th_pid);

    u64 addr_owner_th_pid;
    u32 flags_th;
    int i=0;

    for(i=0;i<MAX_PRIVATE_AREAS;i++){
        // if the addr lies between start_addr and start_addr+length it lies on that thread
        struct thread_private_map* prv_map = &th->private_mappings[i];
        if ( prv_map->start_addr <= addr && prv_map->start_addr+ prv_map->length >= addr) {
              addr_owner_th_pid = prv_map->owner->pid;
              flags_th = prv_map->flags;
        }
    }

    // if the thread calling is different from thread which own the addr
    if( addr_owner_th_pid !=  th_pid) {
        if( error_code = 0x4 && flags_th == 0x13){
            segfault_exit(current->pid, current->regs.entry_rip, addr);
            return -1;
        }
        else if( error_code = 0x6 && (flags_th == 0x13 || flags_th == 0x23)){
            segfault_exit(current->pid, current->regs.entry_rip, addr);
            return -1;
        }
        else if( error_code = 0x7){
            segfault_exit(current->pid, current->regs.entry_rip, addr);
            return -1;
        }
        else if( error_code == 0x4){

            // fix the issue with the page fault-- not understood doing this
            return 1;
        }   
    }
    else {
        // fix the issue with the page fault-- not understood doing this
        return 1;
    }

    segfault_exit(current->pid, current->regs.entry_rip, addr);
    return -1;
}

/*This is a handler called from scheduler. The 'current' refers to the outgoing context and the 'next' 
 * is the incoming context. Both of them can be either the parent process or one of the threads, but only
 * one of them can be the process (as we are having a system with a single user process). This handler
 * should apply the mapping rules passed in the gmalloc calls. */

int handle_private_ctxswitch(struct exec_context *current, struct exec_context *next)
{
  
   /* your implementation goes here*/
   return 0;	

}
