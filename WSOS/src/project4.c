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
    // Verify that both FAT copies are identical.
    for (uint32 i = 0; i < FAT_ENTRIES_MAX; i++)
    {
        if (fat0->clusters[i] != fat1->clusters[i])
        {
            print("Error: FAT copies are inconsistent!\n");
            return 1;
        }
    }

    // Write the first FAT copy (sectors 1-11).
    if (floppy_write(0, FAT0_SECTOR, fat0, FAT_SIZE / 512) != 0)
    {
        print("Error: Failed to write FAT copy 0!\n");
        return 1;
    }

    // Write the second FAT copy (sectors 12-22).
    if (floppy_write(0, FAT1_SECTOR, fat1, FAT_SIZE / 512) != 0)
    {
        print("Error: Failed to write FAT copy 1!\n");
        return 1;
    }

    return 0;
}

int writeCurrentDirectory()
{
    if (!currentDirectory.isOpened || currentDirectory.startingAddress == 0 || currentDirectory.directoryEntry == 0)
    {
        print("Error: No directory is currently open!\n");
        return 1;
    }

    uint16 cluster = currentDirectory.directoryEntry->startingCluster;

    // The root directory occupies sector 23.
    if (cluster == 0)
    {
        if (floppy_write(0, ROOT_DIR_SECTOR, currentDirectory.startingAddress, 1) != 0)
        {
            print("Error: Failed to write the root directory!\n");
            return 1;
        }
        return 0;
    }

    // Write each cluster in the directory's FAT chain.
    uint32 sectorOffset = 0;

    while (cluster != 0xFFFF)
    {
        if (cluster < 2 || cluster >= FAT_ENTRIES_MAX)
        {
            print("Error: Invalid directory cluster!\n");
            return 1;
        }

        if (fat0->clusters[cluster] != fat1->clusters[cluster])
        {
            print("Error: Directory FAT entries are inconsistent!\n");
            return 1;
        }

        uint32 sector = (uint32)cluster + CLUSTER_TO_SECTOR;

        if (floppy_write(0, sector, currentDirectory.startingAddress + (sectorOffset * 512), 1) != 0)
        {
            print("Error: Failed to write a directory cluster!\n");
            return 1;
        }

        cluster = fat0->clusters[cluster];
        sectorOffset++;

        if (sectorOffset > FAT_ENTRIES_MAX)
        {
            print("Error: Invalid or circular directory chain!\n");
            return 1;
        }
    }

    return 0;
}

int writeCurrentFile()
{
    if (!currentFile.isOpened ||
        currentFile.startingAddress == 0 ||
        currentFile.directoryEntry == 0)
    {
        print("Error: No file is currently open!\n");
        return 1;
    }

    uint16 cluster = currentFile.directoryEntry->startingCluster;
    uint32 sectorOffset = 0;

    // Write each cluster in the file's FAT chain.
    while (cluster != 0xFFFF)
    {
        if (cluster < 2 || cluster >= FAT_ENTRIES_MAX)
        {
            print("Error: Invalid file cluster!\n");
            return 1;
        }

        if (fat0->clusters[cluster] != fat1->clusters[cluster])
        {
            print("Error: File FAT entries are inconsistent!\n");
            return 1;
        }

        uint32 sector = (uint32)cluster + CLUSTER_TO_SECTOR;

        if (floppy_write(0, sector, currentFile.startingAddress +
                         (sectorOffset * 512), 1) != 0)
        {
            print("Error: Failed to write a file cluster!\n");
            return 1;
        }

        cluster = fat0->clusters[cluster];
        sectorOffset++;

        if (sectorOffset > FAT_ENTRIES_MAX)
        {
            print("Error: Invalid or circular file chain!\n");
            return 1;
        }
    }

    return 0;
}

int resizeFile(uint16 startingCluster, uint32 newFileSize)
{
    // Calculate how many 512-byte clusters are required.
    uint32 requiredClusters =
        (newFileSize + 511) / 512;

    if (requiredClusters < 1 ||
        requiredClusters > FAT_ENTRIES_MAX - 2)
    {
        print("Error: Invalid requested file size!\n");
        return 1;
    }

    if (startingCluster < 2 ||
        startingCluster >= FAT_ENTRIES_MAX)
    {
        print("Error: Invalid starting cluster!\n");
        return 1;
    }

    // Check the entire FAT before making any changes.
    for (uint32 i = 0; i < FAT_ENTRIES_MAX; i++)
    {
        if (fat0->clusters[i] != fat1->clusters[i])
        {
            print("Error: FAT copies are inconsistent!\n");
            return 1;
        }
    }

    // Count the clusters currently assigned to this file.
    uint32 currentClusters = 0;
    uint16 cluster = startingCluster;

    while (1)
    {
        if (cluster < 2 || cluster >= FAT_ENTRIES_MAX)
        {
            print("Error: Invalid cluster in file chain!\n");
            return 1;
        }

        currentClusters++;

        if (currentClusters > FAT_ENTRIES_MAX - 2)
        {
            print("Error: Circular or oversized file chain!\n");
            return 1;
        }

        uint16 next = fat0->clusters[cluster];

        if (next == 0xFFFF)
            break;

        // A zero entry indicates a broken chain.
        if (next == 0)
        {
            print("Error: File chain ends at a free cluster!\n");
            return 1;
        }

        cluster = next;
    }

    // No allocation changes are necessary.
    if (requiredClusters == currentClusters)
        return 0;

    // Shrink the file by releasing unused clusters.
    if (requiredClusters < currentClusters)
    {
        cluster = startingCluster;

        // Find the cluster that should become the new final cluster.
        for (uint32 i = 1; i < requiredClusters; i++)
        {
            cluster = fat0->clusters[cluster];
        }

        uint16 releasedCluster = fat0->clusters[cluster];

        // Terminate the shortened chain.
        fat0->clusters[cluster] = 0xFFFF;
        fat1->clusters[cluster] = 0xFFFF;

        // Release all remaining clusters.
        while (releasedCluster != 0xFFFF)
        {
            if (releasedCluster < 2 ||
                releasedCluster >= FAT_ENTRIES_MAX)
            {
                print("Error: Invalid cluster while shrinking file!\n");
                return 1;
            }

            uint16 next = fat0->clusters[releasedCluster];

            fat0->clusters[releasedCluster] = 0;
            fat1->clusters[releasedCluster] = 0;

            releasedCluster = next;
        }
    }
    else
    {
        // Verify that sufficient free clusters exist before changing
        // the FAT chain.
        uint32 neededClusters = requiredClusters - currentClusters;
        uint32 freeClusters = 0;

        for (uint32 i = 2; i < FAT_ENTRIES_MAX; i++)
        {
            if (fat0->clusters[i] == 0)
                freeClusters++;
        }

        if (freeClusters < neededClusters)
        {
            print("Error: Not enough free clusters to grow file!\n");
            return 1;
        }

        // Find the existing final cluster.
        cluster = startingCluster;

        while (fat0->clusters[cluster] != 0xFFFF)
        {
            cluster = fat0->clusters[cluster];
        }

        // Append new clusters to the chain.
        for (uint32 i = 0; i < neededClusters; i++)
        {
            uint16 newCluster = nextFreeCluster();

            if (newCluster < 2 ||
                newCluster >= FAT_ENTRIES_MAX)
            {
                print("Error: Could not allocate a free cluster!\n");
                return 1;
            }

            fat0->clusters[cluster] = newCluster;
            fat1->clusters[cluster] = newCluster;

            fat0->clusters[newCluster] = 0xFFFF;
            fat1->clusters[newCluster] = 0xFFFF;

            cluster = newCluster;
        }
    }

    // Persist both FAT copies.
    return writeFATs();
}
