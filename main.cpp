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
                        cout << "The bit map says block " << *ptr << " is free but you found it referenced in the BitMap." << endl;
                        retValue = true;
                        ptr++;
                    }
                }
            }
            if ((inodeArray[i]->addrs[j] != 0) && (mapVec[inodeArray[i]->addrs[j]] == 0) ) {
                cout << "The bit map says block " << inodeArray[i]->addrs[j] << " is free but you found it referenced in the BitMap." << endl;
                retValue = true;
            }
        }
    }
    cout << endl;
    return retValue;
}

bool multipleAllocatedBlock(void * fs, superblock * sb, dinode ** inodeArray, vector<int>mapVec) {
    bool retVal = false;
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
// Checking inode block
// ---------------------------------
    cout << "INODE TYPE" << endl;
    cout << "-----------------------" << endl;
    dinode * inodeArray[sb->ninodes];                   // Array of Inode
    dinode* node = (dinode*)((char*) fs + 1024);        // Pointer at the beinging of the Inode Block
    cout << "Inode Array Number of Links:" << endl;
    dinode* tempNode = node;                            // Temp Pointer to walk through Inode Array to save start of Array.
    for (int i = 0; i < sb->ninodes; i++) {
        inodeArray[i] = tempNode;                           // Pointer at current node = inodeArray at index i
        cout << tempNode++ -> nlink << ' ';
    }
    cout << endl << endl;
    //-----
    tempNode = node;
    cout << "Inode Array Type:" << endl;
    for (int i = 0; i < sb->ninodes; i++) {
        inodeArray[i] = tempNode;
        cout << tempNode++ -> type << ' ';
    }
    //-----
    cout << endl << endl;
    tempNode = node;
    cout << "Size of each File from Inode Array:" << endl;
    for (int i = 0; i < sb->ninodes; i++) {
        inodeArray[i] = tempNode;
        cout << tempNode++ -> size << ' ';
    }
    cout << endl;
    cout << "-----------------------" << endl;
    cout << endl;

// ---------------------------------
// Checking BitMap block
// ---------------------------------
    cout << "BIT MAP" << endl;
    cout << "-----------------------" << endl;
    char * mapPtr = (char*)fs + (((sb->ninodes / IPB) + 1) * BSIZE);
    mapPtr += 1024;                                         // Got help with this from a friend
    
    vector<int> mapVec = {0};
    cout << "BitMap Starting Position: " << mapPtr - (char*)fs << endl;
    cout << "  ";
    for (uint i = 0; i < ((*sb).nblocks / 8); i++) {
       for(int j = 0; j < 8; j++){
            mapVec.push_back(!!(((*mapPtr) << (7 - j)) & 0x80));                // Pushing the correct bits into a vector called mapVec that is the BitMap
            cout << mapVec[i * 8 + j] << " ";
            if((j + 1) % 8 == 0)
                cout << "| ";
            if((i * 8 + j + 1) % 40 == 0){
                cout << endl;
                cout << "| ";
            }
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