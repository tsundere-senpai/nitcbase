#include "StaticBuffer.h"

unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];

StaticBuffer::StaticBuffer()
{
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