#include "project3.h"
#include "multitasking.h"
#include "processes.h"

// An array to hold all of the processes we create
proc_t processes[MAX_PROCS];
 
// Keep track of the next index to place a newly created process in the process array
uint8 process_index = 0;
 
proc_t *prevprocess = 0;       // The previously ran user process
proc_t *runningprocess;    // The currently running process, can be either kernel or user process
proc_t *nextprocess;       // The next process to run
proc_t *kernelprocess;     // The kernel process

#if PROJECT == 3

void proca()
{
    putchar('A');
    exit();
}

void prockernel()
{
    print("Kernel process has started...\n");

	// Create the user processes
	createuserprocess(proca, (void *) 0x10000);

	// Schedule the next process
	int userprocs = ready_process_count();

	// As long as we have ready user processes to run
	while(userprocs > 0)
	{
		// Yield to them
		yield();
		userprocs = ready_process_count();
	}

    print("\nKernel process has exited...\n");
    exit();
}

int kernel()
{
    startkernel(prockernel);
    return 0;
}

#endif

// Select the next user process (proc_t *next) to run
// Selection must be made from the processes array (proc_t processes[])
int schedule()
{
    int count = 0;
    return count;
}

// Yield the current process
// This will give another process a chance to run
// If we yielded a user process, switch to the kernel process
// If we yielded a kernel process, switch to the next process
// The next process should have already been selected via scheduling
void yield()
{
}

// Terminate the process that is currently running (proc_t current)
// Assign the kernel as the next process to run
// Context switch to the kernel process
void exit()
{
}

// Create a new user process
// When the process is eventually ran, start executing from the function provided (void *func)
// Initialize the stack top and base at location (void *stack)
// If we have hit the limit for maximum processes, return -1
// Store the newly created process inside the processes array (proc_t processes[])
int createuserprocess(void *func, void *stack)
{
    (void)func; // Remove this when implementing your function
    (void)stack; // Remove this when implementing your function
    return 0;
}