#pragma once
#include "types.h"
#include "io.h"
#include "processes.h"

//#define PROJECT 3

#if PROJECT == 3
int kernel();
#endif

int schedule();
void yield(); // give up CPU
void exit(); //Terminate current process
int createuserprocess(void *func, void *stack); //create user process

void prockernel(); //function executed by kernel process
void proca(); //function for five user proccesses
void procb();
void procc();
void procd();
void proce();
