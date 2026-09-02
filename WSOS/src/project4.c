#include "project4.h"

// FAT Copies
// First copy is fat0 stored at 
// Second copy is fat1
// There were issues declaring the FATs as non-pointers
// When they would get read from floppy, it would overwrite wrong areas of memory
fat_t *fat0;
fat_t *fat1;

directory_t currentDirectory;  // The current directory we have opened
directory_entry_t rootDirectoryEntry;   // The root directory's directory entry (this does not exist on the disk since the root is not inside of another directory)
file_t currentFile;            // The current file we have opened

#if PROJECT == 4
int kernel()
{
    startkernel(startshell);
    return 0;
}
#endif

int writeFATs()
{
    return 0;
}

int writeCurrentDirectory()
{
    return 0;
}

int writeCurrentFile()
{
    return 0;
}

int resizeFile(uint16 startingCluster, uint32 newFileSize)
{
    (void)startingCluster; // Remove this when implementing your function
    (void)newFileSize; // Remove this when implementing your function
    return 0;
}