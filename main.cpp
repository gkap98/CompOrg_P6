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
    superblock * sb = (superblock*) ((char*)fs + 512);
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
    dinode * inodeArray[sb->ninodes];
    int nodes[sb->ninodes];
    dinode* node = (dinode*)((char*) fs + 1024);        // Pointer at the beinging of the Inode Block
    for (int i = 0; i < sb->ninodes; i++) {
        nodes[i] = node->type;
        if (i % 8 == 0)
            cout << '|';
        cout << node++->type;                           // Printing out type of Inode for every Inode
        if ((i + 1) % 40 == 0 && i != 0)
            cout << endl;
    }
    cout << endl;
    cout << "Inode Array:" << endl;
    for (int i = 0; i < sb->ninodes; i++) {
        inodeArray[i] = node;                           // Pointer at current node = inodeArray at index i
        node += 64;                                     // Node = node + 64 bytes for the size of each inode.
    }
    cout << endl;
    cout << "-----------------------" << endl;
    cout << endl;
// ---------------------------------
// Checking BitMap block
// ---------------------------------
    cout << "BIT MAP" << endl;
    cout << "-----------------------" << endl;
    char * mapPtr;
    if (sb->ninodes % IPB == 0)
        mapPtr = (char*) fs + (((sb->ninodes)/IPB) * 512);          // Setting pointer at the begining of BitMap if number of Inodes fits number of blocks perfectly
    else 
        mapPtr = (char*) fs + ((((sb->ninodes)/IPB) + 1) * 512);    // Setting pointer at the begining of BitMap if number of Inodes does not fit number of blocks perfectly

    vector<int> mapVec = {0};
    char * itr = mapPtr;
    for (int i = 0; i < 2; i++) {
        mapVec[i] = *itr;
        itr++;
    }

    cout << endl;
    return 0;
}