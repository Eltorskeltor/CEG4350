
#include "project5.h"

#if PROJECT == 5
int kernel()
{
    startkernel(startshell);
    return 0;
}
#endif

// Memory location used to load non-root directories.
#define DIRECTORY_ADDR 0x40000


int createSubdirectory(char *subdirectoryName)
{
    // 1. Make sure a directory is currently open.
    if (!currentDirectory.isOpened ||
        currentDirectory.startingAddress == 0 ||
        currentDirectory.directoryEntry == 0)
    {
        print("Error: No directory is currently open!\n");
        return 1;
    }

    // 2. Find the next available directory entry.
    directory_entry_t *emptyEntry = (directory_entry_t *)currentDirectory.startingAddress;

    uint32 entryCount = 0;
    uint32 directoryCapacity =
        currentDirectory.directoryEntry->fileSize / 32;

    while (entryCount < directoryCapacity && emptyEntry->filename[0] != 0)
    {
        emptyEntry++;
        entryCount++;
    }

    // 3. Determine whether the directory needs to grow.
    uint32 directorySizeNeeded = (entryCount + 1) * 32;

    uint32 currentSize =
        currentDirectory.directoryEntry->fileSize;

    char isRootDirectory =
        currentDirectory.directoryEntry->startingCluster == 0;

    // The root directory cannot be resized.
    if (directorySizeNeeded > currentSize && isRootDirectory)
    {
        print("Error: The root directory is full!\n");
        return 1;
    }

    // Resize a non-root directory if necessary.
    if (directorySizeNeeded > currentSize)
    {
        // Resize to the next multiple of 512 bytes.
        uint32 newSize =
            ((directorySizeNeeded + 511) / 512) * 512;

        int resizeFailed = resizeFile(
            currentDirectory.directoryEntry->startingCluster,
            newSize
        );

        if (resizeFailed)
        {
            print("Error: Could not resize the directory!\n");
            return 1;
        }

        // Update the directory size.
        currentDirectory.directoryEntry->fileSize = newSize;

        // Clear the newly allocated memory.
        for (uint32 i = currentSize; i < newSize; i++)
        {
            currentDirectory.startingAddress[i] = 0;
        }
    }

    // 4. Find the next free cluster.
    uint16 next = nextFreeCluster();

    if (next < 2 || next >= FAT_ENTRIES_MAX)
    {
        print("Error: Could not find a free cluster!\n");
        return 1;
    }

    // Make sure both FAT copies agree that this cluster is free.
    if (fat1->clusters[next] != 0)
    {
        print("Error: FAT copies are inconsistent!\n");
        return 1;
    }

    // Mark the cluster as the end of the FAT chain.
    fat0->clusters[next] = 0xFFFF;
    fat1->clusters[next] = 0xFFFF;

    // Write both FAT copies to disk.
    if (writeFATs() != 0)
    {
        print("Error: Could not write the FATs!\n");
        return 1;
    }

    // 5. Populate the new directory entry.
    stringcopy("        ", (char *)emptyEntry->filename, 8);
    stringcopy("   ", (char *)emptyEntry->ext, 3);

    stringcopy(subdirectoryName,
               (char *)emptyEntry->filename, 8);

    // Set the directory attribute.
    emptyEntry->attributes = 0x10;

    // Set the new directory's starting cluster.
    emptyEntry->startingCluster = next;

    // Each new directory initially occupies one cluster.
    emptyEntry->fileSize = 512;

    // Write the parent directory to disk.
    if (writeCurrentDirectory() != 0)
    {
        print("Error: Could not write the parent directory!\n");
        return 1;
    }

    // 6. Create the new directory's contents.
    uint8 buffer[512] = {0};

    // Create the "." entry.
    directory_entry_t *selfEntry = (directory_entry_t *)buffer;

    // Copy the new directory's entry into itself.
    *selfEntry = *emptyEntry;

    // Change the filename to ".".
    stringcopy(".       ",
               (char *)selfEntry->filename, 8);

    // Create the ".." entry.
    directory_entry_t *parentEntry =
        ((directory_entry_t *)buffer) + 1;

    // Copy the current directory's entry.
    *parentEntry = *currentDirectory.directoryEntry;

    // Change the filename to "..".
    stringcopy("..      ",
               (char *)parentEntry->filename, 8);

    // Write the new directory's first cluster to disk.
    floppy_write(0, next + CLUSTER_TO_SECTOR, (void *)buffer, 512);
    {
        print("Error: Could not write the new directory!\n");
        return 1;
    }

    return 0;
}


int openDirectory(char *directoryName)
{
    // 1. Make sure a directory is currently open.
    if (!currentDirectory.isOpened ||
        currentDirectory.startingAddress == 0 ||
        currentDirectory.directoryEntry == 0)
    {
        print("Error: No directory is currently open!\n");
        return 1;
    }

    // 2. Make sure no file is currently open.
    if (currentFile.isOpened)
    {
        print("Error: A file is currently open!\n");
        return 1;
    }

    // 3. Pad the requested directory name with spaces.
    char paddedName[] = "        ";

    uint32 index = 0;

    while (directoryName[index] != 0 && index < 8)
    {
        paddedName[index] = directoryName[index];
        index++;
    }

    // Find the first directory entry.
    directory_entry_t *directoryEntry =
        (directory_entry_t *)currentDirectory.startingAddress;

    char directoryExists = 0;

    // Search until we find the requested directory.
    while (directoryEntry->filename[0] != 0)
    {
        // Check the filename and the directory attribute.
        directoryExists =
            stringcompare(
                (char *)directoryEntry->filename,
                paddedName,
                8
            ) &&
            ((directoryEntry->attributes & 0x10) == 0x10);

        if (directoryExists)
        {
            break;
        }

        directoryEntry++;
    }

    // Return if the directory could not be found.
    if (!directoryExists)
    {
        print("Error: Could not find the requested directory!\n");
        return 1;
    }

    // Save a copy before loading the new directory.
    // This is important because the new directory may overwrite
    // the memory currently holding the old directory.
    directory_entry_t targetEntry = *directoryEntry;

    // 4. Check whether we are opening the root directory.
    if (targetEntry.startingCluster == 0)
    {
        // Point to the root directory entry.
        currentDirectory.directoryEntry = &rootDirectoryEntry;

        // Set the root directory's memory address.
        currentDirectory.startingAddress = (uint8 *)ROOT_ADDR;

        // Read the root directory from disk.
        int readFailed = floppy_read(
            0,
            ROOT_DIR_SECTOR,
            (void *)currentDirectory.startingAddress,
            512
        );

        if (readFailed != 0)
        {
            print("Error: Could not read the root directory!\n");
            return 1;
        }

        currentDirectory.isOpened = 1;
        currentDirectory.index = 0;

        return 0;
    }

    // 5. Opening a non-root directory.

    // Set the memory location for the new directory.
    uint8 *startingAddress = (uint8 *)DIRECTORY_ADDR;

    // Start at the directory's first cluster.
    uint16 cluster = targetEntry.startingCluster;

    uint32 sectorCount = 0;

    // Traverse the FAT chain and load each cluster.
    while (cluster != 0xFFFF)
    {
        // Check for invalid clusters.
        if (cluster < 2 || cluster >= FAT_ENTRIES_MAX)
        {
            print("Error: Invalid directory cluster!\n");
            return 1;
        }

        // Check that both FAT copies agree.
        if (fat0->clusters[cluster] != fat1->clusters[cluster])
        {
            print("Error: FAT copies are inconsistent!\n");
            return 1;
        }

        // Convert the cluster number to a sector.
        uint32 sector = cluster + CLUSTER_TO_SECTOR;

        // Read one 512-byte cluster into memory.
        int readFailed = floppy_read(
            0,
            sector,
            startingAddress + (512 * sectorCount),
            512
        );

        if (readFailed != 0)
        {
            print("Error: Could not read a directory cluster!\n");
            return 1;
        }

        sectorCount++;

        // Get the next cluster in the FAT chain.
        cluster = fat0->clusters[cluster];

        // Prevent infinite loops from corrupted FAT chains.
        if (sectorCount > FAT_ENTRIES_MAX)
        {
            print("Error: Invalid or circular directory chain!\n");
            return 1;
        }
    }

    // 6. Find the "." entry inside the loaded directory.
    directory_entry_t *selfEntry = (directory_entry_t *)startingAddress;

    directory_entry_t *foundSelf = 0;

    uint32 entryCount = sectorCount * 16;

    for (uint32 i = 0; i < entryCount; i++)
    {
        // Check for the "." entry.
        if (selfEntry->filename[0] == '.' &&
            selfEntry->filename[1] == ' ')
        {
            foundSelf = selfEntry;
            break;
        }

        selfEntry++;
    }

    if (foundSelf == 0)
    {
        print("Error: Could not find the self directory entry!\n");
        return 1;
    }

    // 7. Update the current directory information.
    currentDirectory.startingAddress = startingAddress;

    // Point to the "." entry.
    currentDirectory.directoryEntry = foundSelf;

    currentDirectory.isOpened = 1;
    currentDirectory.index = 0;

    return 0;
}
