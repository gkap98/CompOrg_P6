#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <vector>
#include "types.h"
#include "fs.h"

using namespace std;

#define T_DIR  1   // Directory
#define T_FILE 2   // File
#define T_DEV  3   // Special device

// --------------------------------------
// File System Check Functions
// --------------------------------------
bool unalocatedBlock(void * fs, superblock * sb, dinode ** inodeArray, vector<int>mapVec) {
    for (uint i = 0; i < sb->ninodes; i++) {
        for (int j = 0; j < NDIRECT + 1; j++) {
            if (inodeArray[i]->addrs[j] > mapVec.size()) {
                continue;
            }
            else if (j == NDIRECT) {
                uint indirectBlock = (inodeArray[i]->addrs[j]);
                if (indirectBlock > (sb->nblocks)) {        // Make sure that the indirectBlock is greater than the number of blocks
                    break;
                }
                if (inodeArray[i]->addrs[j] == 1) {
                    continue;
                }
                uint * ptr = (uint *)fs + (indirectBlock * BSIZE) / 4;
                for (int k = 0; k < 128; k++) {                 // k is going through the total number of possible indirect address blocks.
                    if (*ptr > sb->nblocks) {
                        cout << "ERROR: Inode address points to an invalid block" << endl;
                        return true;
                    }
                    // Cheching the indirect block of addresses
                    if ((*ptr != 0) && (mapVec[*ptr] == 0)) {
                        cout << "ERROR: The bit map says block " << *ptr << " is free but you found it referenced in a valid Inode." << endl;
                        return true;
                    }
                    ptr++;
                }
            }
            // Chechig the direct block of addresses
            else if ((inodeArray[i]->addrs[j + 1] != 0) && (mapVec[inodeArray[i]->addrs[j + 1]] == 0) ) {
                cout << "ERROR: The bit map says block " << inodeArray[i]->addrs[j] << " is free but you found it referenced in a valid Inode." << endl;
                return true;
            }
        }
    }
    return false;
}

bool multipleAllocatedBlock(void * fs, superblock * sb, dinode ** inodeArray, vector<int>mapVec) {
    int blockLog[sb->nblocks];
    for (uint i = 0; i < sb->nblocks; i++) {
        blockLog[i] = 0;
    }
    for (uint i = 0; i < sb->ninodes; i++) {
        for (uint j = 0; j < NDIRECT + 1; j++) {
            if (inodeArray[i]->addrs[j] > mapVec.size()) {
                continue;
            }
            else if (j == NDIRECT) {
                uint indirectBlock = (inodeArray[i]->addrs[j]);
                if (indirectBlock > (sb->nblocks)) {            // Make sure that the indirectBlock is greater than the number of blocks * 8
                    break;
                }
                if (inodeArray[i]->addrs[j] == 1) {
                    continue;
                }
                uint * ptr = (uint *)fs + (indirectBlock * BSIZE) / 4;
                for (int k = 0; k < 128; k++) {                 // k is going through the total number of possible indirect address blocks.
                    if (*ptr > sb->nblocks) {
                        cout << "ERROR: Inode address points to an invalid block" << endl;
                        return true;
                    }
                    blockLog[*ptr]++;
                    ptr++;
                }
            }
            else {
                blockLog[inodeArray[i]->addrs[j]]++;
            }
        }
    }
    for (uint i = 1; i < sb->nblocks; i++) {
        if (blockLog[i] > 1) {
            //cout << blockLog[i] << endl;
            cout << "ERROR: Block " << i << " is referenced by more than one Inode" << endl;
            return true;
        }
    }
    return false;
}

bool fileSizeTooBig(void * fs, superblock * sb, dinode ** inodeArray, vector<int>mapVec) {
    for (uint i = 0; i < sb->ninodes; i++) {
        if (MAXFILE * BSIZE < inodeArray[i]->size) {
            cout << "ERROR: Inode " << i << "'s size in bytes exceeds the maximum file size" << endl;
            return true;
        }
    }
    return false;
}

bool slashProblems(void * fs, superblock * sb, dinode ** inodeArray, vector<int>mapVec) {
    if (mapVec[1] == 0) {
        cout << "ERROR: Inode 1 is not allocated" << endl;
        return true;
    }
    if (inodeArray[1]->type != 1) {
        cout << "ERROR: Inode 1 is not a directory" << endl;
        return true;
    }
    dirent * dir = (dirent*)((char*)fs + (BSIZE *(*inodeArray[1]->addrs)));
    dirent* nameOne = dir;
    dirent* nameTwo = dir;
    nameTwo++;
    if (((string)(nameOne->name) != ".") || nameOne->inum != 1 || nameTwo->inum != 1 || ((string)(nameTwo->name) != "..")) {
        cout << "ERROR: Root directory does not refer to itself as . and .." << endl;
        return true;
    }
    return false;
}

bool firstTwoNamesOfFile(void * fs, superblock * sb, dinode ** inodeArray, vector<int>mapVec) {
    for (uint i = 1; i < sb->ninodes; i++) {
        if (inodeArray[i]->type == 1) {
            dirent * dir = (dirent*)((char*)fs + (BSIZE *(*inodeArray[i]->addrs)));
            dirent* nameOne = dir;
            dirent* nameTwo = dir;
            nameTwo++;
        
            if (((string)nameOne->name != ".") || ((string)nameTwo->name != "..")) {
                cout << "ERROR: Inode " << i << "'s first two file directory names are not . and .." << endl;
                return true;
            }
        }
    }
    return false;
}

bool inodeOneNotRoot(void * fs, superblock * sb, dinode ** inodeArray, vector<int>mapVec) {
    for (uint i = 2; i < sb->ninodes; i++) {
        if (inodeArray[i]->type == 1) {
            dirent * dir = (dirent*)((char*)fs + (BSIZE *(*inodeArray[i]->addrs)));
            if (dir->inum == 1) 
            {
                cout << "ERROR: Inode " << i << " reffers to Inode 1 as ." << endl;
                return true;
            }
        }
    }
    return false;
}

bool missingBlock(void * fs, superblock * sb, dinode ** inodeArray, vector<int>mapVec) {
    
    vector<int> referenceToBlock;
    for (uint i = 0;  i< sb->ninodes; i++) {        // Going through the number of inodes
        for (uint j = 0; j < NDIRECT + 1; j++) {
            if (j == NDIRECT) {                                             // Making sure that the NDIRECTS are equal to the index of j
                uint indirect = inodeArray[i]->addrs[j];                    // Creating a indirect variable that is equal to the address of the inode its pointing to 
                referenceToBlock.push_back(inodeArray[i]->addrs[j]);
                uint * ptr = (uint *)fs + (indirect * BSIZE) / 4;           // Pointer at the start of the indeirect pointers
                for (int k = 0; k < 128; k++) {
                    referenceToBlock.push_back(*ptr);                       // Pushing derefferenced pointer to the vector if the block is refferenced
                    ptr++;                                                  // Incrementing pointer 
                }
            }
            else {
                referenceToBlock.push_back(inodeArray[i]->addrs[j]);        // else push on the rest of the inode addresses.
            }
        }
    }
    bool found = false;                                                     // variable to change if a allocated block is found
    for (uint i = sb->bmapstart + 1; i < mapVec.size(); i++) {
        if (mapVec[i] == 1) {
            found = false;
            for (uint j = 0; j < referenceToBlock.size(); j++) {
                if (referenceToBlock[j] == i) {
                    found = true;                                           // If you do find an Inode refferenced to the correctly allocated block
                    break;
                }
            }
            if (found == false) {                                           // If you can't find the correct match, then the error has occured
                cout << "The bit map says block " << i << " is allocated but you cannot find it belonging to any file or directory." << endl;
                return true;
            }
        }
    }
    cout << endl;
    return false;
}

bool superblockCheck(void * fs, superblock * sb, dinode ** inodeArray, vector<int>mapVec) {
    
}

// --------------------------------------
// End of File System Check Functions
// --------------------------------------

int main(int argc, char * argv[]) {

    int fd = -1;
    // Making sure file system was givin in command line correctly
    if ((fd = open(argv[1], O_RDONLY)) < 0) {
		perror("File System Failed To Be Read");
        exit(1);
    }
    // Reading in File System at 512*1024 size
    void * fs = mmap(nullptr, BSIZE * 1024, PROT_READ, MAP_SHARED, fd, 0);
    if(fs == MAP_FAILED) {
        perror("Unable to access File System");
        munmap(fs, BSIZE*1024);
        exit(1);
    }

// ---------------------------------
// Checking of the Superblock
// ---------------------------------

    // Creating a Superblock pointer to check Supberblock size
    superblock * sb = (superblock*) ((char*)fs + BSIZE);
    struct stat fileSystem;
    stat(argv[1], &fileSystem); 
    // If Superblock is not the correct size
    if (fileSystem.st_size/BSIZE != sb->size) {                             // If the file system size is less than the actual size of the file system
        cout << endl;
        cout << "File System Error" << endl;
        cout << "Incorrect size of Superblock" << endl;
        cout << "Superblock size: " << sb->size << endl;
        cout << "-----------------------" << endl;
        cout << "SUPERBLOCK -> NOT OK" << endl;
        cout << endl;
        munmap(fs, BSIZE*1024);                                             // Deallocating Memory
        exit(1);
    }
    else {                                                                  // Else print out the qualities that are needed 
        cout << endl;
        cout << "SUPERBLOCK" << endl;
        cout << "-----------------------" << endl;
        cout << "   Size: " << sb->size << endl;
        cout << "   Number of Blocks: " << sb->nblocks << endl;
        cout << "   Number of Inodes: " << sb->ninodes << endl;
        cout << "   Number of Logs:   " << sb->nlog << endl;
        cout << "------------"          << endl;                            
        cout << "   Logs begins at:   " << sb->logstart << endl;
        cout << "   Inodes begins at: " << sb->inodestart << endl;
        cout << "   BitMap begins at: " << sb->bmapstart << endl;
        cout << "-----------------------" << endl;  
        cout << "SUPERBLOCK -> OK" << endl;                                 // Stating the clearence of the Superblock checker
        cout << endl;
    }

// ---------------------------------
// Inode block
// ---------------------------------

    dinode * inodeArray[sb->ninodes];                               // Array of Inode
    for (int i = 0; i < sb->ninodes; i++) {
        inodeArray[i] = NULL;                                       // Initializing inodes with NULL to prevent garbage values
    }
    dinode * node = (dinode*)((char*) fs + (sb->inodestart * BSIZE));                    // Pointer at the beinging of the Inode Block
    for (uint i = 0; i < sb->ninodes; i++) {
        inodeArray[i] = node++;                                     // Filling inodeArray with Inodes
    }

// ---------------------------------
// BitMap block
// ---------------------------------

    char * mapPtr = (char*)(fs) + (sb->bmapstart * BSIZE);
    vector<int> mapVec;                                       // Vector to hold the BitMap data
    for (uint i = 0; i < (sb->nblocks / 8) + 1; i++) {
       for(int j = 0; j < 8; j++){
            mapVec.push_back(!!(((*mapPtr) << (7 - j)) & 0x80));      // Pushing the correct bits into a vector called mapVec that is the BitMap
            cout << mapVec[i] << ' ';
       }
        mapPtr++;                                                   // Incrementing the pointer to the BitMap
    }
    cout << endl;

// ---------------------------------
// Catching File System Errors
// ---------------------------------
    cout << endl;
    cout << "Current File System Errors Found: " << endl;
    cout << "-----------------------------------------" << endl;

    if(unalocatedBlock(fs, sb, inodeArray, mapVec)) {
        munmap(fs, BSIZE*1024);
        exit(1);
    }
    else if(multipleAllocatedBlock(fs, sb, inodeArray, mapVec)) {
        munmap(fs, BSIZE*1024);
        exit(1);
    }
    else if(fileSizeTooBig(fs, sb, inodeArray, mapVec)) {
        munmap(fs, BSIZE*1024);
        exit(1);
    }
    else if(inodeOneNotRoot(fs, sb, inodeArray, mapVec)) {
        munmap(fs, BSIZE*1024);
        exit(1);
    }
    else if(slashProblems(fs, sb, inodeArray, mapVec)) {
        munmap(fs, BSIZE*1024);
        exit(1);
    }
    else if(firstTwoNamesOfFile(fs, sb, inodeArray, mapVec)) {
        munmap(fs, BSIZE*1024);
        exit(1);
    }
    else if(missingBlock(fs, sb, inodeArray, mapVec)) {
        munmap(fs, BSIZE*1024);
        exit(1);
    }

    else {
        cout << endl << "No System Errors Found" << endl << endl;
        munmap(fs, BSIZE*1024);
    }
    cout << endl;
    return 0;
}
