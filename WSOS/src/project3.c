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

void procb()
{
    putchar('B');
    yield();

    putchar('B');
    exit();
}

void procc()
{
    putchar('C');
    yield();

    putchar('C');
    yield();

    putchar('C');
    yield();

    putchar('C');
    exit();
}

void procd()
{
    putchar('D');
    yield();

    putchar('D');
    yield();

    putchar('D');
    exit();
}

void proce()
{
    putchar('E');
    yield();

    putchar('E');
    exit();
}

void prockernel()
{
    
    print("Kernel process has started...\n");

	// Create the user processes
    createuserprocess(proca, (void *)0x10000);
    createuserprocess(procb, (void *)0x11000);
    createuserprocess(procc, (void *)0x12000);
    createuserprocess(procd, (void *)0x13000);
    createuserprocess(proce, (void *)0x14000);

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
    int previous_pid = -1;
    int i;

    if (prevprocess != 0) //if a process already ran, get its pid to compare with the next process
    {
        previous_pid = prevprocess->pid;
    }

    for (i = 0; i < MAX_PROCS; i++) //find lowest PID of a user process that is ready to run and has a PID greater than the previous process
    {
        if (processes[i].type == PROC_TYPE_USER &&
            processes[i].status == PROC_STATUS_READY &&
            processes[i].pid > previous_pid)
        {
            nextprocess = &processes[i];
            return 1;
        }
    }

    for (i = 0; i < MAX_PROCS; i++) //if no user process has a PID greater than the previous process, find the lowest PID of a user process that is ready to run
    {
        if (processes[i].type == PROC_TYPE_USER &&
            processes[i].status == PROC_STATUS_READY)
        {
            nextprocess = &processes[i];
            return 1;
        }
    }

    return 0; //no user processes are ready to run
}

// Yield the current process
// This will give another process a chance to run
// If we yielded a user process, switch to the kernel process
// If we yielded a kernel process, switch to the next process
// The next process should have already been selected via scheduling
void yield()
{
    if (runningprocess->type == PROC_TYPE_USER) //if the current process is a user process
    {
        // The user process is still alive and can run again.
        runningprocess->status = PROC_STATUS_READY;

        // Remember which user process just ran.
        prevprocess = runningprocess;

        // The kernel must run next so it can schedule.
        nextprocess = kernelprocess;

        contextswitch();
        return;
    }

    if (runningprocess->type == PROC_TYPE_KERNEL) //if the current process is a kernel process
    {
        // Find the next ready user process.
        if (schedule())
        {
            contextswitch();
        }
    }
}

// Terminate the process that is currently running (proc_t current)
// Assign the kernel as the next process to run
// Context switch to the kernel process
void exit()
{
    if (runningprocess->type == PROC_TYPE_KERNEL) //if the current process is a kernel process
    {
        return;
    }

    runningprocess->status = PROC_STATUS_TERMINATED; // Mark the current process as terminated

    // The kernel must run next.
    nextprocess = kernelprocess;

    contextswitch();
}

// Create a new user process
// When the process is eventually ran, start executing from the function provided (void *func)
// Initialize the stack top and base at location (void *stack)
// If we have hit the limit for maximum processes, return -1
// Store the newly created process inside the processes array (proc_t processes[])
int createuserprocess(void *func, void *stack)
{
    proc_t *process;

    if (process_index >= MAX_PROCS) //if we have hit the limit for maximum processes
    {
        return -1;
    }

    process = &processes[process_index]; //get the next available process slot in the processes array

    process->pid = process_index; // Assign a unique process ID based on the current index
    process->type = PROC_TYPE_USER; // Set the process type to user
    process->status = PROC_STATUS_READY; // Set the process status to ready

    process->esp = stack; // Set the stack pointer to the provided stack location
    process->ebp = stack; // Set the base pointer to the provided stack location

    process->eip = func; // Set the instruction pointer to the provided function

    process_index++; // Increment the process index for the next process

    return 0;
}
