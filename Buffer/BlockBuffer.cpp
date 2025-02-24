#include "BlockBuffer.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
// the declarations for these functions can be found in "BlockBuffer.h"

BlockBuffer::BlockBuffer(int blockNum)
{
    // initialise this.blockNum with the argument
    this->blockNum = blockNum;
}

// calls the parent class constructor
RecBuffer::RecBuffer(int blockNum) : BlockBuffer::BlockBuffer(blockNum) {}

// load the block header into the argument pointer
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
    memcpy(&head->numSlots, bufferptr + 24, 4);
    memcpy(&head->numEntries, bufferptr + 16, 4);
    memcpy(&head->numAttrs, bufferptr + 20, 4);
    memcpy(&head->rblock, bufferptr + 12, 4);
    memcpy(&head->lblock, bufferptr + 8, 4);
    memcpy(&head->pblock, bufferptr + 4, 4);
    return SUCCESS;
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
    struct HeadInfo head;
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
} /*
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
        if (bufferNum == FAILURE || bufferNum==E_OUTOFBOUND)
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
