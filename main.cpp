#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include "types.h"
#include "fs.h"
#include <sys/stat.h>
using namespace std;


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
        munmap(fs, BSIZE*512);
        exit(1);
    }

// ---------------------------------
// Checking SIZE of Superblock
// ---------------------------------
    // Creating a Superblock pointer to check Supberblock size
    superblock * sb = (superblock*) ((char*)fs + 512);
    struct stat fileSystem;
    stat(argv[1], &fileSystem); 
    if (fileSystem.st_size/BSIZE != sb->size) {
        cout << "File System Error" << endl;
        cout << "Incorrect size of Superblock" << endl;
        cout << "Superblock size: " << sb->size << endl;
        munmap(fs, BSIZE*512);
        exit(1);
    }
    else {
        cout << endl;
        cout << "SUPERBLOCK" << endl;
        cout << "-----------------------" << endl;
        cout << "Size: " << sb->size << endl;
        cout << "Number of Blocks: " << sb->nblocks << endl;
        cout << "Number of inodes: " << sb->ninodes << endl;
    }
// ---------------------------------
// Checking inode block
// ---------------------------------
    


    cout << endl;
    return 0;
}