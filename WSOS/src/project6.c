#include "project6.h"

#if PROJECT == 6
int kernel()
{
    startkernel(startshell);
    return 0;
}
#endif

// Delete the file that is currently opened
// All FAT entries for this file must be assigned a value of x0000
// Set the directory entry's first byte to x00
// Copy the last directory entry into this one if there are other files in this directory
// Returns 0 if file was successfully deleted
// Returns non-zero if error occurred
int deleteFile()
{
    return 0;
}

// Delete the subdirectory inside of the directory that is currently opened
// Check to see if the subdirectory exists and is empty
// All FAT entries for this subdirectory must be assigned a value of x0000
// Set the directory entry's first byte to x00
// Copy the last directory entry into this one if there are other files in this directory
// Returns 0 if subdirectory was successfully deleted
// Returns non-zero if error occurred
int deleteSubdirectory(char *directoryName)
{
    (void)directoryName; // Remove this when implementing your function
    return 0;
}