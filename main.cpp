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

bool unalocatedBlock(void * fs, superblock * sb, dinode ** inodeArray, vector<int>mapVec) {
    bool retValue = false;
    for (uint i = 0; i < sb->ninodes; i++) {
        for (int j = 0; j < NDIRECT + 1; j++) {
            if (j == NDIRECT) {
                uint indirectBlock = (inodeArray[i]->addrs[j]) * BSIZE;
                if (indirectBlock > (sb->nblocks * 8)) {        // Make sure that the indirectBlock is greater than the number of blocks * 8
                    break;
                }
                uint * ptr = (uint *)fs + indirectBlock;
                for (int k = 0; k < 128; k++) {                 // k is going through the total number of possible indirect address blocks.
                    if ((*ptr != 0) && (mapVec[*ptr] == 0)) {
                        cout << "The bit map says block " << *ptr << " is free but you found it referenced in a valid Inode." << endl;
                        return true;
                    }
                    ptr++;
                }
            }
            else if ((inodeArray[i]->addrs[j] != 0) && (mapVec[inodeArray[i]->addrs[j]] == 0) ) {
                cout << "The bit map says block " << inodeArray[i]->addrs[j] << " is free but you found it referenced in a valid Inode." << endl;
                return true;
            }
        }
    }
    cout << endl;
    return retValue;
}

bool multipleAllocatedBlock(void * fs, superblock * sb, dinode ** inodeArray, vector<int>mapVec) {
    bool retValue = false;
    int blockLog[sb->nblocks];
    for (int i = 0; i < sb->nblocks; i++) {
        blockLog[i] = 0;
    }
    for (uint i = 0; i < sb->ninodes; i++) {
        for (int j = 0; j < NDIRECT + 1; j++) {
            if (j == NDIRECT) {
                uint indirectBlock = (inodeArray[i]->addrs[j]) * BSIZE;
                if (indirectBlock > (sb->nblocks * 8)) {        // Make sure that the indirectBlock is greater than the number of blocks * 8
                    break;
                }
                uint * ptr = (uint *)fs + indirectBlock;
                for (int k = 0; k < 128; k++) {                 // k is going through the total number of possible indirect address blocks.
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
        //cout << i << ' ' << blockLog[i] << endl;
        if (blockLog[i] > 1) {
            cout << "Block is referenced by more than one Inode" << endl;
            retValue = true;
        }
    }
    cout << endl;
    return retValue;
}

int main(int argc, char * argv[]) {
    int fd = -1;
    // Making sure file system was givin in command line correctly
    if ((fd = open(argv[1], O_RDONLY)) < 0) {
		perror("File System Failed To Be Read");
        exit(1);
    }
    
    // Reading in File System at 512*1024 size
    void * fs = mmap(nullptr, BSIZE*1024, PROT_READ, MAP_SHARED, fd, 0);
    if(fs == MAP_FAILED) {
        perror("Unable to access File System");
        munmap(fs, BSIZE*1024);
        exit(1);
    }

// ---------------------------------
// Checking SIZE of Superblock
// ---------------------------------
    // Creating a Superblock pointer to check Supberblock size
    superblock * sb = (superblock*) ((char*)fs + BSIZE);
    struct stat fileSystem;
    stat(argv[1], &fileSystem); 
    // If Superblock is not the correct size
    if (fileSystem.st_size/BSIZE != sb->size) {
        cout << "File System Error" << endl;
        cout << "Incorrect size of Superblock" << endl;
        cout << "Superblock size: " << sb->size << endl;
        munmap(fs, BSIZE*1024);
        exit(1);
    }
    else {
        cout << endl;
        cout << "SUPERBLOCK" << endl;
        cout << "-----------------------" << endl;
        cout << "Size: " << sb->size << endl;
        cout << "Number of Blocks: " << sb->nblocks << endl;
        cout << "Number of inodes: " << sb->ninodes << endl;
        cout << "-----------------------" << endl;
        cout << endl;
    }
// ---------------------------------
// Inode block
// ---------------------------------

    dinode * inodeArray[sb->ninodes];                   // Array of Inode
    for (int i = 0; i < sb->ninodes; i++) {
        inodeArray[i] = NULL;
    }
    dinode* node = (dinode*)((char*) fs + 1024);        // Pointer at the beinging of the Inode Block
    for (uint i = 0; i < sb->ninodes; i++) {
        inodeArray[i] = node++;
    }

// ---------------------------------
// BitMap block
// ---------------------------------
    char * mapPtr = (char*)fs + (((sb->ninodes / IPB) + 3) * BSIZE);
    vector<int> mapVec = {0};
    for (uint i = 0; i < ((*sb).nblocks / 8); i++) {
       for(int j = 0; j < 8; j++){
            mapVec.push_back(!!(((*mapPtr) << (7 - j)) & 0x80));    // Pushing the correct bits into a vector called mapVec that is the BitMap
       }
        mapPtr++;
    }
    
// ---------------------------------
// Catching File System Errors
// ---------------------------------
    cout << endl << endl;
    cout << "Current File System Errors Found: " << endl;
    cout << "-----------------------------------------" << endl;
    unalocatedBlock(fs, sb, inodeArray, mapVec);
    multipleAllocatedBlock(fs, sb, inodeArray, mapVec);
    cout << endl;
    return 0;
}