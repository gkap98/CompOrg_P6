# CompOrg_P6
    File System Bug Check

# Address Arithmatic Rules
    Pointer or Address Arithmatic, is always in the units of the size of the type of pointer.
    If pointer is 'unsigned char*' then moving to the next inode is pointer += sizeof(inode).
        or
    If pointer is 'inode*' then moving to the next inode is pointer++.

# Space between Inode block and BitMap block
    #define BBLOCK(b, ninodes) (b/BPB + (ninodes)/IPB + 3)

