#include <debug.h>
#include <context.h>
#include <entry.h>
#include <lib.h>
#include <memory.h>


/*****************************HELPERS******************************************/

/*
 * allocate the struct which contains information about debugger
 *
 */
struct debug_info *alloc_debug_info()
{
	struct debug_info *info = (struct debug_info *) os_alloc(sizeof(struct debug_info));
	if(info)
		bzero((char *)info, sizeof(struct debug_info));
	return info;
}
/*
 * frees a debug_info struct
 */
void free_debug_info(struct debug_info *ptr)
{
	if(ptr)
		os_free((void *)ptr, sizeof(struct debug_info));
}



/*
 * allocates a page to store registers structure
 */
struct registers *alloc_regs()
{
	struct registers *info = (struct registers*) os_alloc(sizeof(struct registers));
	if(info)
		bzero((char *)info, sizeof(struct registers));
	return info;
}

/*
 * frees an allocated registers struct
 */
void free_regs(struct registers *ptr)
{
	if(ptr)
		os_free((void *)ptr, sizeof(struct registers));
}

/*
 * allocate a node for breakpoint list
 * which contains information about breakpoint
 */
struct breakpoint_info *alloc_breakpoint_info()
{
	struct breakpoint_info *info = (struct breakpoint_info *)os_alloc(
		sizeof(struct breakpoint_info));
	if(info)
		bzero((char *)info, sizeof(struct breakpoint_info));
	return info;
}

/*
 * frees a node of breakpoint list
 */
void free_breakpoint_info(struct breakpoint_info *ptr)
{
	if(ptr)
		os_free((void *)ptr, sizeof(struct breakpoint_info));
}

/*
 * Fork handler.
 * The child context doesnt need the debug info
 * Set it to NULL
 * The child must go to sleep( ie move to WAIT state)
 * It will be made ready when the debugger calls wait
 */
void debugger_on_fork(struct exec_context *child_ctx)
{
	// printk("DEBUGGER FORK HANDLER CALLED\n");
	child_ctx->dbg = NULL;
	child_ctx->state = WAITING;
}


/******************************************************************************/


/* This is the int 0x3 handler
 * Hit from the childs context
 */
void copy_regs(struct registers* regs, struct user_regs user_regs,int inc_value){//} int end_handler_flag,) {
    
    regs->entry_rip = user_regs.entry_rip - 1;
    regs->entry_rsp = user_regs.entry_rsp;
    regs->rbp = user_regs.rbp;
    regs->rax = user_regs.rax;

	regs->rdi = user_regs.rdi;
    regs->rsi = user_regs.rsi;
    //if(end_handler_flag == 0) regs->rdx = user_regs.rdx;
	//else 
	regs->rdx = user_regs.rdx + inc_value;
    regs->rcx = user_regs.rcx;
    regs->r8 = user_regs.r8;
    regs->r9 = user_regs.r9;

	regs->entry_cs = user_regs.entry_cs;
    regs->entry_rflags = user_regs.entry_rflags;
    regs->entry_ss = user_regs.entry_ss;

    regs->rbx = user_regs.rbx;
    regs->r15 = user_regs.r15;
    regs->r14 = user_regs.r14;
    regs->r13 = user_regs.r13;
    regs->r12 = user_regs.r12;
    regs->r11 = user_regs.r11;
    regs->r10 = user_regs.r10;
    
}

long int3_handler(struct exec_context *ctx)
{	
	
	// //Your code

	//printk("--------------------INT3 begin-------------------\n");
	if(ctx == NULL) return -1;
	
	ctx-> state = WAITING; // child to WAITING

	struct exec_context* parent_debuger =  get_ctx_by_pid(ctx->ppid);
	parent_debuger->state = READY; // parent to READY

	if(parent_debuger->dbg == NULL) {
		//printk("INT3: debugee closed \n");
		ctx->regs.entry_rsp -= 8;// ctx->regs.entry_rsp - sizeof(ctx->regs.rbp); // move RSP upward
		*((u64*) ctx->regs.entry_rsp) = ctx->regs.rbp;
        schedule(ctx);
	}

	
	u64 breakpt_addr = ctx->regs.entry_rip -1; 
	parent_debuger->regs.rax = breakpt_addr; // update wait_and_continue return value;
	//printk("-----------Breakpoint addr = %x-----------\n",breakpt_addr);
	if(breakpt_addr == (u64) (parent_debuger->dbg->end_handler)) {
		if (parent_debuger->dbg->child_regs) {
			free_regs(parent_debuger->dbg->child_regs);
		}
		parent_debuger->dbg->child_regs = alloc_regs();
		copy_regs(parent_debuger->dbg->child_regs, ctx->regs,parent_debuger->dbg->inc_value);

		parent_debuger->dbg->inc_value -= 8; // extra space in stack used earlier removed

		(parent_debuger->dbg->stack_top) --;
		(ctx->regs).entry_rsp -= 8;// ctx->regs.entry_rsp - sizeof((u64)ctx->regs.rbp);
        *((u64*) ctx->regs.entry_rsp) = (ctx->regs).rbp;
	}
	else {

		if (parent_debuger->dbg->child_regs) {
			free_regs(parent_debuger->dbg->child_regs);
		}
		parent_debuger->dbg->child_regs = alloc_regs();
		copy_regs(parent_debuger->dbg->child_regs, ctx->regs,parent_debuger->dbg->inc_value);

		int breakpt_end_flag = -1;
		struct breakpoint_info * curr_node = parent_debuger->dbg->head;
		while(curr_node != NULL){
			//printk("In int3, searching breakpoint, brkpoint addr: %x , curr noede addr: %x\n",breakpt_addr,curr_node->addr);
			if(curr_node->addr == breakpt_addr) { // matched breakpoint
				breakpt_end_flag = curr_node->end_breakpoint_enable;
				break;
			}
			curr_node = curr_node->next;
		}
		//printk("INT3: called from top of function before updating stack, flag = %d\n",breakpt_end_flag);
		if(breakpt_end_flag){
			//store address of function in callstaack
			(parent_debuger->dbg->stack_top)++;
			parent_debuger->dbg->functions_oncallstack[(parent_debuger->dbg->stack_top)] = breakpt_addr;

			// push do_end_handler address in the stack
			(ctx->regs).entry_rsp -= 8;// ctx->regs.entry_rsp - sizeof((u64)ctx->regs.rbp);
        	*((u64*) ctx->regs.entry_rsp) = (u64)(parent_debuger->dbg->end_handler);
			parent_debuger->dbg->inc_value += 8; // stack moved 8 up
		}
		ctx->regs.entry_rsp -= 8;// ctx->regs.entry_rsp - sizeof((u64)ctx->regs.rbp);
        *((u64*) ctx->regs.entry_rsp) = (ctx->regs).rbp;

	}

	//---- backtrace update-----
	if(! parent_debuger->dbg->backtrace) { // free backtrace to update
		os_free(parent_debuger->dbg->backtrace,(MAX_BACKTRACE + 1)*sizeof(u64));
	}
	parent_debuger->dbg->backtrace = (u64*) os_alloc ((MAX_BACKTRACE + 1)*sizeof(u64));

	u64 rbp_value = ctx->regs.entry_rsp; // rbp currently at top
    u64* callq_ret_ptr = (u64*) (rbp_value + 0x8); // ret_ptr is one below rbp

	// if do_end_handler() address is stored then go one more down
    if (*callq_ret_ptr == (u64) parent_debuger->dbg->end_handler) {
        callq_ret_ptr++;
    }
    int i = 0;
	// breakpoint not called from do_end__handler()
    if (breakpt_addr != (u64) parent_debuger->dbg->end_handler) {
        parent_debuger->dbg->backtrace[i] = breakpt_addr;
        i++;
    }

    while ( i < MAX_BACKTRACE && *callq_ret_ptr != END_ADDR) {  //end as per defination
        parent_debuger->dbg->backtrace[i] = *callq_ret_ptr;
        rbp_value = *((u64*) rbp_value);
        callq_ret_ptr = (u64*) (rbp_value + 0x8);
        if (*callq_ret_ptr == (u64) parent_debuger->dbg->end_handler) {
            callq_ret_ptr++;
        }
		//printk("INT3, Backtrace: i=%d, stored address: %x\n",i,*callq_ret_ptr);
        i++;
    }
    parent_debuger->dbg->backtrace[i] = END_ADDR;

	schedule(parent_debuger);
	return 0;


}

/*
 * Exit handler.
 * Deallocate the debug_info struct if its a debugger.
 * Wake up the debugger if its a child
 */
void debugger_on_exit(struct exec_context *ctx)
{
	// Your code
	// exit everyting in child 
	if(ctx->dbg == NULL){ // ctx of child
		struct exec_context *parent_debuger = get_ctx_by_pid(ctx->ppid);
		parent_debuger->regs.rax = CHILD_EXIT;
		parent_debuger->state = READY;
	}
	else{
		struct breakpoint_info* curr_node=ctx->dbg->head;
		struct breakpoint_info *temp;
		while( curr_node != NULL){
			temp = curr_node;
			curr_node = curr_node->next;
			free_breakpoint_info(temp);
		}
		free_debug_info(ctx->dbg);		
	}
	
}


/*
 * called from debuggers context
 * initializes debugger state
 */
int do_become_debugger(struct exec_context *ctx, void *addr)
{
	// Your code
	if(!ctx) return -1;

	ctx->dbg = alloc_debug_info();

	if(ctx->dbg == NULL) return -1; // alloc_debug_info() not worked.

	char* inst=(char*)addr;
 	*inst=0xCC;

	ctx->dbg->head = NULL; // breakpoint header = null in intialization
	ctx->dbg->breakpoint_count = 0;
	ctx->dbg->breakpoint_number = 0;
	ctx->dbg->end_handler = addr;
	//ctx->dbg->backtrace = NULL;
	ctx->dbg->stack_top = -1;
	ctx->dbg->inc_value =0;

	// check here .
	//if(ctx->dbg == NULL) printk("------------IN do_become_debugger: ctx dbg is null\n-------------");

	

	return 0;
}

/*
 * called from debuggers context
 */
int do_set_breakpoint(struct exec_context *ctx, void *addr, int flag)
{

	// Your code

	if (!ctx || !(ctx->dbg)) return -1;
	if(addr == NULL) return -1;

	if(ctx->dbg->breakpoint_count >= MAX_BREAKPOINTS) return -1; // already done max number of debuggers set

	if(ctx->dbg->head == NULL){

		char* inst=(char*)addr;
 		*inst=0xCC;

		struct breakpoint_info *temp = alloc_breakpoint_info();
		
		ctx->dbg->breakpoint_count=1;
		ctx->dbg->breakpoint_number++;

		temp->addr = (u64)addr;
		temp->num = ctx->dbg->breakpoint_number;
		temp->end_breakpoint_enable = flag;
		temp->next = NULL;

		ctx->dbg->head = temp;
		return 0;
		
	}


	struct breakpoint_info *curr_node = ctx->dbg->head;
	struct breakpoint_info *prev_node = NULL;

	int top = ctx->dbg->stack_top;
	int i=0;
	int on_callstack=0;

	for(i=0; i<= top;i++){
		if(ctx->dbg->functions_oncallstack[i] == (u64) addr) {
			on_callstack = 1;
		}
	}

	if(ctx->dbg->head->addr == (u64) addr){
		//return 0;
		if (on_callstack == 1) {
			if (curr_node->end_breakpoint_enable == 1 && flag==0)return -1;
		}
		curr_node->end_breakpoint_enable = flag; // update the falg
		//ctx->dbg->breakpoint_number ++; // number of set breakpoint success increment
		return 0;
	}

	while(curr_node!= NULL){
		if(curr_node->addr == (u64)addr){
			//return 0;
			if (on_callstack == 1) {
				if (curr_node->end_breakpoint_enable == 1 && flag==0) return -1;
			}
			curr_node->end_breakpoint_enable = flag; // update the falg
			return 0;
		}
		prev_node = curr_node;
		curr_node = curr_node->next;
	}
	struct breakpoint_info *temp = alloc_breakpoint_info();
	if(temp == NULL) return -1;
	
	char* inst=(char*)addr;
 		*inst=0xCC;
	
	ctx->dbg->breakpoint_number +=1;
	ctx->dbg->breakpoint_count +=1;
	
	temp->addr = (u64)addr;
	temp->num = ctx->dbg->breakpoint_number;
	temp->end_breakpoint_enable = flag;
	temp->next = NULL;
	prev_node->next = temp;

	return 0;

}

/*
 * called from debuggers context
 */
int do_remove_breakpoint(struct exec_context *ctx, void *addr)
{
	//Your code
	if(!ctx || !ctx->dbg || !ctx->dbg->head) return -1;
	if(addr == NULL) return -1;

	int top = ctx->dbg->stack_top;
	int i=0;
	int on_callstack=0;

	for(i=0; i<= top;i++){
		if(ctx->dbg->functions_oncallstack[i] == (u64) addr) {
			//return -1;
			on_callstack = 1;
		}
	}

	struct breakpoint_info *curr_node = ctx->dbg->head;
	struct breakpoint_info *prev_node = NULL;

	while(curr_node != NULL) {
		if(curr_node->addr == (u64)addr){
			
			if (on_callstack && curr_node->end_breakpoint_enable){
				return -1;
			} 
			if(prev_node == NULL) ctx->dbg->head = curr_node->next;
			else prev_node->next = curr_node->next;

			ctx->dbg->breakpoint_count --;
			u64* data = (u64 *)((u64)addr);
			*data = PUSHRBP_OPCODE;
			free_breakpoint_info(curr_node);
			return 0;
		}
		prev_node= curr_node;
		curr_node = curr_node->next;
	}

	return -1; // if not found so not return already
}


/*
 * called from debuggers context
 */

int do_info_breakpoints(struct exec_context *ctx, struct breakpoint *ubp)
{
	
	// Your code
	if(!ctx->dbg || !ubp ) return -1;
	struct breakpoint_info *curr_node = ctx->dbg->head;
	int i=0;
	while(curr_node != NULL){
		if(i == MAX_BREAKPOINTS ) return -1; // more breakpoint then allowed
		ubp[i].addr = curr_node->addr;
		ubp[i].end_breakpoint_enable = curr_node->end_breakpoint_enable;
		ubp[i].num = curr_node->num;
		curr_node = curr_node->next;
		i++;
	}

	return (int)ctx->dbg->breakpoint_count;
}


/*
 * called from debuggers context
 */
int do_info_registers(struct exec_context *ctx, struct registers *regs)
{
	// Your code
	if(!ctx || !regs ) return -1;
	// struct exec_context * child_ctx;
	// int found = 0, i=0;
	// for (i=1;i<MAX_PROCESSES;i++){ // possible pids in GemOS
	// 	child_ctx = get_ctx_by_pid(i);
	// 	if(child_ctx->ppid == ctx->pid) { // found child
	// 		found = 1;
	// 		break;
	// 	}
	// }
	// if(found != 1) return -1; 
	// if(child_ctx == NULL) return -1;
	// store the resistor values of child registers
	regs->entry_rip = ctx->dbg->child_regs->entry_rip;
	regs->entry_rsp = ctx->dbg->child_regs->entry_rsp;
    regs->rbp = ctx->dbg->child_regs->rbp;
    regs->rax = ctx->dbg->child_regs->rax;

    regs->rdi = ctx->dbg->child_regs->rdi;
	regs->rsi = ctx->dbg->child_regs->rsi;
	regs->rdx = ctx->dbg->child_regs->rdx;
	regs->rcx = ctx->dbg->child_regs->rcx;
	regs->r8 = ctx->dbg->child_regs->r8;
	regs->r9 = ctx->dbg->child_regs->r9;

	regs->entry_cs =  ctx->dbg->child_regs->entry_cs;
	regs->entry_rflags = ctx->dbg->child_regs->entry_rflags;
	regs->entry_ss = ctx->dbg->child_regs->entry_ss;

	regs->rbx = ctx->dbg->child_regs->rbx;
    regs->r15 = ctx->dbg->child_regs->r15;
    regs->r14 = ctx->dbg->child_regs->r14;
    regs->r13 = ctx->dbg->child_regs->r13;
    regs->r12 = ctx->dbg->child_regs->r12;
    regs->r11 = ctx->dbg->child_regs->r11;
    regs->r10 = ctx->dbg->child_regs->r10;

	return 0;
}

/*
 * Called from debuggers context
 */
int do_backtrace(struct exec_context *ctx, u64 bt_buf)
{

	// Your code
	if(!ctx || !ctx->dbg || !((u64*) bt_buf)) return -1;

	int i=0;
	while(1){
		if(ctx->dbg->backtrace[i] == END_ADDR) break;
		((u64*) bt_buf)[i] = ctx->dbg->backtrace[i];
		i++;
	}
	return i;
}


/*
 * When the debugger calls wait
 * it must move to WAITING state
 * and its child must move to READY state
 */

s64 do_wait_and_continue(struct exec_context *ctx)
{
	// Your code

	if(!ctx->dbg) return -1;

	ctx->state = WAITING;

	struct exec_context * child_ctx;
	int found = 0, i=0;
	for (i=1; i<MAX_PROCESSES; i++){ // possible pids in GemOS
		child_ctx = get_ctx_by_pid(i);
		if(child_ctx && child_ctx->ppid == ctx->pid) { // found child
			found = 1;
			child_ctx->state= READY;
			break;
		}
	}
	if(found != 1) return -1; // child not found

	schedule(child_ctx);

	//printk("Wait: chilld rip %x \n", child_ctx->regs.entry_rip);
	
	
	return 0;
}






