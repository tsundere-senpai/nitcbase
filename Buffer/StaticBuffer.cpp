#include "StaticBuffer.h"

unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];
unsigned char StaticBuffer::blockAllocMap[DISK_BLOCKS];

StaticBuffer::StaticBuffer()
{
    
        for (int block_index= 0,bmap_slot=0; block_index < 4; block_index++)
        {
            unsigned char buffer[BLOCK_SIZE];
            Disk::readBlock(buffer,block_index);

            for(int slot=0;slot<BLOCK_SIZE;slot++,bmap_slot++){
                StaticBuffer::blockAllocMap[bmap_slot]=buffer[slot];
            }
            
        }
        
    
    for (int i = 0; i < BUFFER_CAPACITY; i++)
    {
        metainfo[i].free = true;
        metainfo[i].dirty = false;
        metainfo[i].blockNum = -1;
        metainfo[i].timeStamp = -1;
    }
}

StaticBuffer::~StaticBuffer()
{
    // do nothing
    for (int block_index = 0,bmap_slot=0; block_index < 4; block_index++)
    {
        unsigned char buffer[BLOCK_SIZE];

        for(int slot=0;slot<BLOCK_SIZE;slot++,bmap_slot++){
            buffer[slot]=StaticBuffer::blockAllocMap[bmap_slot];

        }
        Disk::writeBlock(buffer,block_index);   


    }
    
    for (int i = 0; i < BUFFER_CAPACITY; i++)
    {
        if (metainfo[i].dirty && !metainfo[i].free)
        {

            Disk::writeBlock(blocks[i], metainfo[i].blockNum);
        }
    }
}

int StaticBuffer::setDirtyBit(int blockNum)
{
    int buffIndex = getBufferNum(blockNum);
    if (buffIndex < 0)
    {
        return buffIndex;
    }
    else
    {
        metainfo[buffIndex].dirty = true;
        return SUCCESS;
    }
}
int StaticBuffer::getFreeBuffer(int blockNum)
{
    if (blockNum < 0 || blockNum > DISK_BLOCKS)
    {
        return E_OUTOFBOUND;
    }
    for (int i = 0; i < BUFFER_CAPACITY; i++)
    {
        metainfo[i].timeStamp++;
    }

    int allocatedBuffer = -1;

    for (int i = 0; i < BUFFER_CAPACITY; i++)
    {
        if (metainfo[i].free == true)
        {

            allocatedBuffer = i;
            break;
        }
    }
    if (allocatedBuffer == -1)
    {
        int largest = metainfo[0].timeStamp;
        allocatedBuffer = 0;
        for (int i = 0; i < BUFFER_CAPACITY; i++)
        {
            if (metainfo[i].timeStamp > largest)
            {
                largest = metainfo[i].timeStamp;
                allocatedBuffer = i;
            }
        }
        if (metainfo[allocatedBuffer].dirty)
        {
            Disk::writeBlock(blocks[allocatedBuffer], metainfo[allocatedBuffer].blockNum);
        }
    }
    metainfo[allocatedBuffer].dirty = false;
    metainfo[allocatedBuffer].free = false;
    metainfo[allocatedBuffer].timeStamp = 0;
    metainfo[allocatedBuffer].blockNum = blockNum;
    return allocatedBuffer;
}

int StaticBuffer::getBufferNum(int blockNum)
{
    if (blockNum < 0 || blockNum > DISK_BLOCKS)
    {
        return E_OUTOFBOUND;
    }
    for (int i = 0; i < BUFFER_CAPACITY; i++)
    {
        if (metainfo[i].blockNum == blockNum)
        {
            return i;
        }
    }
    return E_BLOCKNOTINBUFFER;
}