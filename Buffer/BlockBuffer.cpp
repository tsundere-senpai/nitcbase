#include "BlockBuffer.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
// the declarations for these functions can be found in "BlockBuffer.h"
BlockBuffer::BlockBuffer(char blockType)
{
    // allocate a block on the disk and a buffer in memory to hold the new block of
    // given type using getFreeBlock function and get the return error codes if any.
    int blocktype;
    switch (blockType)
    {
    case 'R':
        /* code */
        blocktype = REC;
        break;
    case 'I':
        blocktype = IND_INTERNAL;
        break;
    case 'L':
        blocktype = IND_LEAF;
        break;
    default:
        blocktype = UNUSED_BLK;
        break;
    }
    int blocknum = BlockBuffer::getFreeBlock(blocktype);
    // set the blockNum field of the object to that of the allocated block
    // number if the method returned a valid block number,
    // otherwise set the error code returned as the block number.
    if (blocknum < 0)
    {
        std::cout << "Error: Block is not available\n";
        this->blockNum = blocknum;
        return;
    }
    this->blockNum = blocknum;

    // (The caller must check if the constructor allocatted block successfully
    // by checking the value of block number field.)
}
BlockBuffer::BlockBuffer(int blockNum)
{
    // initialise this.blockNum with the argument
    this->blockNum = blockNum;
}

//* calls the parent class constructor
RecBuffer::RecBuffer(int blockNum) : BlockBuffer::BlockBuffer(blockNum) {}

//* calls parent non-default constructor with 'R' denoting record block.
RecBuffer::RecBuffer() : BlockBuffer('R') {}

//* calls the corresponding parent constructor
IndBuffer::IndBuffer(char blockType) : BlockBuffer(blockType) {}

//* calls the corresponding parent constructor
IndBuffer::IndBuffer(int blockNum) : BlockBuffer(blockNum) {}

//* calls the corresponding parent constructor
//* 'I' used to denote IndInternal.
IndInternal::IndInternal() : IndBuffer('I') {}

//* calls the corresponding parent constructor
IndInternal::IndInternal(int blockNum) : IndBuffer(blockNum) {}

//* this is the way to call parent non-default constructor.
//* 'L' used to denote IndLeaf.
IndLeaf::IndLeaf() : IndBuffer('L') {}

//* this is the way to call parent non-default constructor.
IndLeaf::IndLeaf(int blockNum) : IndBuffer(blockNum) {}

int BlockBuffer::getHeader(HeadInfo *head)
{

    unsigned char *bufferptr;
    int ret = loadBlockAndGetBufferPtr(&bufferptr);
    if (ret != SUCCESS)
    {
        return ret;
    }
    // copy the header from the buffer to the argument pointer

    // populate the numEntries, numAttrs and numSlots fields in *head
    memcpy(&head->pblock, bufferptr + 4, 4);
    memcpy(&head->lblock, bufferptr + 8, 4);
    memcpy(&head->rblock, bufferptr + 12, 4);
    memcpy(&head->numEntries, bufferptr + 16, 4);
    memcpy(&head->numAttrs, bufferptr + 20, 4);
    memcpy(&head->numSlots, bufferptr + 24, 4);
    return SUCCESS;
}

int BlockBuffer::setHeader(HeadInfo *head)
{

    unsigned char *bufferPtr;
    // get the starting address of the buffer containing the block using
    // loadBlockAndGetBufferPtr(&bufferPtr).
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if (ret != SUCCESS)
        return ret;

    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
    // return the value returned by the call.

    // cast bufferPtr to type HeadInfo*
    struct HeadInfo *bufferHeader = (struct HeadInfo *)bufferPtr;

    // copy the fields of the HeadInfo pointed to by head (except reserved) to
    // the header of the block (pointed to by bufferHeader)
    //(hint: bufferHeader->numSlots = head->numSlots )
    bufferHeader->blockType = head->blockType;
    bufferHeader->lblock = head->lblock;
    bufferHeader->rblock = head->rblock;
    bufferHeader->pblock = head->pblock;
    bufferHeader->numAttrs = head->numAttrs;
    bufferHeader->numEntries = head->numEntries;
    bufferHeader->numSlots = head->numSlots;
    // update dirty bit by calling StaticBuffer::setDirtyBit()
    // if setDirtyBit() failed, return the error code
    ret = StaticBuffer::setDirtyBit(this->blockNum);
    if (ret != SUCCESS)
        return ret;

    return SUCCESS;
    // return SUCCESS;
}
// load the record at slotNum into the argument pointer
int RecBuffer::getRecord(union Attribute *rec, int slotNum)
{

    // copy the record from the buffer to the argument pointer

    HeadInfo head;
    // get the header using this.getHeader() function
    BlockBuffer::getHeader(&head);

    int attrCount = head.numAttrs;
    int slotCount = head.numSlots;

    // read the block at this.blockNum into a buffer
    unsigned char *bufferptr;
    int ret = loadBlockAndGetBufferPtr(&bufferptr);
    if (ret != SUCCESS)
    {
        return ret;
    }

    /* record at slotNum will be at offset HEADER_SIZE + slotMapSize + (recordSize * slotNum)
       - each record will have size attrCount * ATTR_SIZE
       - slotMap will be of size slotCount
    */
    int recordSize = attrCount * ATTR_SIZE;
    unsigned char *slotPointer = bufferptr + (32 + slotCount + (recordSize * slotNum));

    // load the record into the rec data structure
    memcpy(rec, slotPointer, recordSize);

    return SUCCESS;
}
int RecBuffer::setRecord(union Attribute *rec, int slotNum)
{

    unsigned char *buffer;
    int ret = loadBlockAndGetBufferPtr(&buffer);
    if (ret != SUCCESS)
    {
        return ret;
    }
    HeadInfo head;
    // get the header using this.getHeader() function
    BlockBuffer::getHeader(&head);

    int attrCount = head.numAttrs;
    int slotCount = head.numSlots;

    if (slotNum >= slotCount)
        return E_OUTOFBOUND;

    // read the block at this.blockNum into a buffer
    /* record at slotNum will be at offset HEADER_SIZE + slotMapSize + (recordSize * slotNum)
       - each record will have size attrCount * ATTR_SIZE
       - slotMap will be of size slotCount
    */

    int recordSize = attrCount * ATTR_SIZE;
    unsigned char *slotPointer = buffer + (32 + slotCount + (recordSize * slotNum));
    memcpy(slotPointer, rec, recordSize);

    int retu = StaticBuffer::setDirtyBit(this->blockNum);
    if (retu != SUCCESS)
    {
        std::cout << "ERROR\n";
        exit(1);
    }
    // load the record into the rec data structure

    // Disk::writeBlock(buffer, this->blockNum);
    return SUCCESS;
}

int BlockBuffer::setBlockType(int blockType)
{

    unsigned char *bufferPtr;
    /* get the starting address of the buffer containing the block
       using loadBlockAndGetBufferPtr(&bufferPtr). */
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
    // return the value returned by the call.
    if (ret != SUCCESS)
    {
        return ret;
    }

    // store the input block type in the first 4 bytes of the buffer.
    // (hint: cast bufferPtr to int32_t* and then assign it)
    // *((int32_t *)bufferPtr) = blockType;
    int32_t *blockTypePointer = (int32_t *)bufferPtr;
    *blockTypePointer = blockType;

    // update the StaticBuffer::blockAllocMap entry corresponding to the
    // object's block number to `blockType`.
    StaticBuffer::blockAllocMap[this->blockNum] = blockType;
    return StaticBuffer::setDirtyBit(this->blockNum);

    // update dirty bit by calling StaticBuffer::setDirtyBit()
    // if setDirtyBit() failed
    // return the returned value from the call

    // return SUCCESS
}
int BlockBuffer::getFreeBlock(int blockType)
{

    // iterate through the StaticBuffer::blockAllocMap and find the block number
    // of a free block in the disk.
    int i = 0;
    for (; i < DISK_BLOCKS; i++)
    {
        if (StaticBuffer::blockAllocMap[i] == UNUSED_BLK)
            break;
    }
    if (i >= DISK_BLOCKS)
    {
        return E_DISKFULL;
    }
    // if no block is free, return E_DISKFULL.
    // set the object's blockNum to the block number of the free block.
    this->blockNum = i;
    // find a free buffer using StaticBuffer::getFreeBuffer() .
    int bufNum = StaticBuffer::getFreeBuffer(i);
    if (bufNum < 0)
    {
        return bufNum;
    }
    // initialize the header of the block passing a struct HeadInfo with values
    // pblock: -1, lblock: -1, rblock: -1, numEntries: 0, numAttrs: 0, numSlots: 0
    // to the setHeader() function.
    HeadInfo initHead;
    initHead.pblock = -1;
    initHead.lblock = -1;
    initHead.rblock = -1;
    initHead.numAttrs = 0;
    initHead.numSlots = 0;
    initHead.numEntries = 0;
    BlockBuffer::setHeader(&initHead);
    // update the block type of the block to the input block type using setBlockType().
    BlockBuffer::setBlockType(blockType);
    // return block number of the free block.
    return i;
}

/*
 Used to load a block to the buffer and get a pointer to it.
 NOTE: this function expects the caller to allocate memory for the argument
 */
/*
 * This function loads a block into the buffer and gets a pointer to it.
 *
 * Parameters:
 * - buffPtr: A pointer to an unsigned char pointer where the buffer address will be stored.
 *
 * Returns:
 * - SUCCESS: If the block is successfully loaded into the buffer.
 * - E_BLOCKNOTINBUFFER: If the block is not present in the buffer.
 * - E_OUTOFBOUND: If there is no free buffer available.
 */
int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char **buffPtr)
{
    // check whether the block is already present in the buffer using StaticBuffer.getBufferNum()
    int bufferNum = StaticBuffer::getBufferNum(this->blockNum);
    if (bufferNum == E_OUTOFBOUND)
    {
        return E_OUTOFBOUND;
    }
    if (bufferNum != E_BLOCKNOTINBUFFER)
    {
        for (int i = 0; i < BUFFER_CAPACITY; i++)
        {
            StaticBuffer::metainfo[i].timeStamp++;
        }
        StaticBuffer::metainfo[bufferNum].timeStamp = 0;
    }
    else if (bufferNum == E_BLOCKNOTINBUFFER)
    {
        bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);

        // if the block is not present in the buffer, read it from the disk
        if (bufferNum == FAILURE || bufferNum == E_OUTOFBOUND)
        {
            return bufferNum;
        }

        Disk::readBlock(StaticBuffer::blocks[bufferNum], this->blockNum);
    }

    // store the pointer to this buffer (blocks[bufferNum]) in *buffPtr
    *buffPtr = StaticBuffer::blocks[bufferNum];

    return SUCCESS;
}
/* used to get the slotmap from a record block
NOTE: this function expects the caller to allocate memory for `*slotMap`
*/
int RecBuffer::getSlotMap(unsigned char *slotMap)
{
    unsigned char *bufferPtr;

    // get the starting address of the buffer containing the block using loadBlockAndGetBufferPtr().
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if (ret != SUCCESS)
    {
        return ret;
    }

    struct HeadInfo head;
    // get the header of the block using getHeader() function
    BlockBuffer::getHeader(&head);

    int slotCount = head.numSlots;

    // get a pointer to the beginning of the slotmap in memory by offsetting HEADER_SIZE
    unsigned char *slotMapInBuffer = bufferPtr + HEADER_SIZE;

    // copy the values from `slotMapInBuffer` to `slotMap` (size is `slotCount`)
    for (int i = 0; i < slotCount; i++)
    {
        slotMap[i] = slotMapInBuffer[i];
    }

    return SUCCESS;
}
int RecBuffer::setSlotMap(unsigned char *slotMap)
{
    unsigned char *bufferPtr;

    int buffer = loadBlockAndGetBufferPtr(&bufferPtr);

    if (buffer != SUCCESS)
        return buffer;
    HeadInfo head;
    getHeader(&head);
    int numSlots = head.numSlots;

    memcpy(bufferPtr + HEADER_SIZE, slotMap, numSlots);
    int ret = StaticBuffer::setDirtyBit(this->blockNum);
    if (ret != SUCCESS)
    {
        return ret;
    }
    return SUCCESS;
}
int BlockBuffer::getBlockNum()
{

    return this->blockNum;
    // return corresponding block number.
}
void BlockBuffer::releaseBlock()
{

    // if blockNum is INVALID_BLOCKNUM (-1), or it is invalidated already, do nothing
    if (this->blockNum == INVALID_BLOCKNUM ||
        StaticBuffer::blockAllocMap[this->blockNum] == UNUSED_BLK)
        return;
    else
    {
        int bufferNum = StaticBuffer::getBufferNum(this->blockNum);
        if (bufferNum < 0)
        {
            return;
        }
        if (bufferNum >= 0 && bufferNum < BUFFER_CAPACITY)
            StaticBuffer::metainfo[bufferNum].free = true;

        StaticBuffer::blockAllocMap[this->blockNum] = UNUSED_BLK;
        this->blockNum = INVALID_BLOCKNUM;
    }
    // else
    /* get the buffer number of the buffer assigned to the block
       using StaticBuffer::getBufferNum().
       (this function return E_BLOCKNOTINBUFFER if the block is not
       currently loaded in the buffer)
        */

    // if the block is present in the buffer, free the buffer
    // by setting the free flag of its StaticBuffer::tableMetaInfo entry
    // to true.

    // free the block in disk by setting the data type of the entry
    // corresponding to the block number in StaticBuffer::blockAllocMap
    // to UNUSED_BLK.

    // set the object's blockNum to INVALID_BLOCK (-1)
}
int compareAttrs(union Attribute attr1, union Attribute attr2, int attrType)
{

    double diff;
    /*
    if diff > 0 then return 1
    if diff < 0 then return -1
    if diff = 0 then return 0
    */
    if (attrType == STRING)
    {
        diff = strcmp(attr1.sVal, attr2.sVal);
    }
    else
        diff = attr1.nVal - attr2.nVal;

    if (diff > 0)
        return 1;
    if (diff < 0)
        return -1;
    return 0;
}

int IndInternal::getEntry(void *ptr, int indexNum) {
    // if the indexNum is not in the valid range of [0, MAX_KEYS_INTERNAL-1]
    //     return E_OUTOFBOUND.
	if (indexNum < 0 || indexNum >= MAX_KEYS_INTERNAL) return E_OUTOFBOUND;

    unsigned char *bufferPtr;
    /* get the starting address of the buffer containing the block
       using loadBlockAndGetBufferPtr(&bufferPtr). */
	int ret = loadBlockAndGetBufferPtr(&bufferPtr);

    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
    //     return the value returned by the call.
	if (ret != SUCCESS) return ret;

    // typecast the void pointer to an internal entry pointer
    struct InternalEntry *internalEntry = (struct InternalEntry *)ptr;

    // TODO: copy the entries from the indexNum`th entry to *internalEntry
    //* make sure that each field is copied individually as in the following code
    //* the lChild and rChild fields of InternalEntry are of type int32_t
    //* int32_t is a type of int that is guaranteed to be 4 bytes across every
    //* C++ implementation. sizeof(int32_t) = 4

    // the indexNum'th entry will begin at an offset of
	// HEADER_SIZE + (indexNum * (sizeof(int) + ATTR_SIZE) ) from bufferPtr
        
	unsigned char *entryPtr = bufferPtr + HEADER_SIZE + (indexNum * 20);

    memcpy(&(internalEntry->lChild), entryPtr, sizeof(int32_t));
    memcpy(&(internalEntry->attrVal), entryPtr + 4, sizeof(Attribute));
    memcpy(&(internalEntry->rChild), entryPtr + 20, sizeof(int32_t));

    return SUCCESS; 
}

int IndLeaf::getEntry(void *ptr, int indexNum) 
{
    // if the indexNum is not in the valid range of [0, MAX_KEYS_LEAF-1]
    //     return E_OUTOFBOUND.
	if (indexNum < 0 || indexNum >= MAX_KEYS_LEAF) return E_OUTOFBOUND;

    unsigned char *bufferPtr;
    /* get the starting address of the buffer containing the block
       using loadBlockAndGetBufferPtr(&bufferPtr). */
	int ret = loadBlockAndGetBufferPtr(&bufferPtr);

    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
    //     return the value returned by the call.
	if (ret != SUCCESS) return ret;

    // copy the indexNum'th Index entry in buffer to memory ptr using memcpy

    // the indexNum'th entry will begin at an offset of
    // HEADER_SIZE + (indexNum * LEAF_ENTRY_SIZE)  from bufferPtr
    
	unsigned char *entryPtr = bufferPtr + HEADER_SIZE + (indexNum * LEAF_ENTRY_SIZE);
    memcpy((struct Index *)ptr, entryPtr, LEAF_ENTRY_SIZE);
    

    return SUCCESS;
}
int IndLeaf::setEntry(void *ptr, int indexNum) {

    // if the indexNum is not in the valid range of [0, MAX_KEYS_LEAF-1]
    //     return E_OUTOFBOUND.
    if(indexNum<0||indexNum>=MAX_KEYS_LEAF)
        return E_OUTOFBOUND;

    unsigned char *bufferPtr;
    /* get the starting address of the buffer containing the block
       using loadBlockAndGetBufferPtr(&bufferPtr). */
    int ret=loadBlockAndGetBufferPtr(&bufferPtr);

    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
    //     return the value returned by the call.
    if (ret!=SUCCESS)
    {
        /* code */
        return ret;
    }
    
    // copy the Index at ptr to indexNum'th entry in the buffer using memcpy
    /* the indexNum'th entry will begin at an offset of
       HEADER_SIZE + (indexNum * LEAF_ENTRY_SIZE)  from bufferPtr */
    unsigned char *entryPtr = bufferPtr + HEADER_SIZE + (indexNum * LEAF_ENTRY_SIZE);
    memcpy(entryPtr, (struct Index *)ptr, LEAF_ENTRY_SIZE);

    // update dirty bit using setDirtyBit()
    // if setDirtyBit failed, return the value returned by the call
    return StaticBuffer::setDirtyBit(this->blockNum);
    
    //return SUCCESS
}
int IndInternal::setEntry(void *ptr, int indexNum) {
    // if the indexNum is not in the valid range of [0, MAX_KEYS_INTERNAL-1]
    //     return E_OUTOFBOUND.

    if(indexNum<0||indexNum>=MAX_KEYS_INTERNAL)
        return E_OUTOFBOUND;

    unsigned char *bufferPtr;
    /* get the starting address of the buffer containing the block
       using loadBlockAndGetBufferPtr(&bufferPtr). */
    int ret=loadBlockAndGetBufferPtr(&bufferPtr);
    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
    //     return the value returned by the call.
    if (ret!=SUCCESS)
    {
        return ret;
    }
    
    // typecast the void pointer to an internal entry pointer
    struct InternalEntry *internalEntry = (struct InternalEntry *)ptr;

    /*
    - copy the entries from *internalEntry to the indexNum`th entry
    - make sure that each field is copied individually as in the following code
    - the lChild and rChild fields of InternalEntry are of type int32_t
    - int32_t is a type of int that is guaranteed to be 4 bytes across every
      C++ implementation. sizeof(int32_t) = 4
    */

    /* the indexNum'th entry will begin at an offset of
       HEADER_SIZE + (indexNum * (sizeof(int) + ATTR_SIZE) )         [why?]
       from bufferPtr */

    unsigned char *entryPtr = bufferPtr + HEADER_SIZE + (indexNum * 20);

    memcpy(entryPtr, &(internalEntry->lChild), 4);
    memcpy(entryPtr + 4, &(internalEntry->attrVal), ATTR_SIZE);
    memcpy(entryPtr + 20, &(internalEntry->rChild), 4);


    // update dirty bit using setDirtyBit()
    // if setDirtyBit failed, return the value returned by the call
    ret=StaticBuffer::setDirtyBit(this->blockNum);
    if(ret!=SUCCESS)
        return ret;

    return SUCCESS;
    // return SUCCESS
}