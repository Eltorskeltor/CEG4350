#pragma once
#include "types.h"
#include "io.h"

#if PROJECT == 1
int kernel();
#endif

int setcursor(int x, int y);