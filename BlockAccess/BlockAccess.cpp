#include "BlockAccess.h"

#include <cstring>

RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op)
{
    // get the search index for the relation
    RecId prevRecId;
    RelCacheTable::getSearchIndex(relId, &prevRecId);

    int block, slot;
    // if the search index is not set, start from the first block
    if (prevRecId.block == -1 && prevRecId.slot == -1)
    {
        // get the first block of the relation
        //we use the relCatEntry to get the first block of the relation
        //because the first block of the relation is stored in the relCatEntry
        RelCatEntry relCatEntry;
        RelCacheTable::getRelCatEntry(relId, &relCatEntry);
        block = relCatEntry.firstBlk;
        // start from the first slot
        slot = 0;
    }
    else // start from the next record
    {
        //set block to the block number of the previous record
        block = prevRecId.block;
        //set slot to the slot number of the previous record + 1 ie the next record (incrementing the slot number)
        slot = prevRecId.slot + 1;
    }
    // loop through all the blocks of the relation
    while (block != -1)
    {
        // get the block buffer
        //we initialise the blockBuffer with the block number
        //this is because we want to access the block
        RecBuffer blockBuffer(block);
        // get the header of the block
        HeadInfo blockHead;
        blockBuffer.getHeader(&blockHead);
        // get the record at the current slot
        //using headInfo.numAttrs to get the number of attributes in the block to set the size of the record array
        Attribute record[blockHead.numAttrs];
        //we already initialised the blockBuffer with the block number
        //using the blockBuffer to get the record at the current slot
        //we pass the record array and the slot number to the getRecord function
        //this function will fill the record array with the record at the slot number
        blockBuffer.getRecord(record, slot);


        //we can now get the slot map of the block
        unsigned char slotMap[blockHead.numSlots];
        //we use getslotmap from blockBuffer to get the slot map of the block
        //this function will fill the slotMap array with the slot map of the block
        //it will use loadBlockAndGetBufferPtr to get the buffer of the block
        //and then get the header of the block
        //it will get numofslots from header and then set a unsigned char pointer to the slot map in the buffer using the header size(32)
        //it will then get the slot map from the buffer and copy it to the slotMap array using a for loop
        blockBuffer.getSlotMap(slotMap);

        // if the slot is greater than the number of slots in the block, move to the next block
        if (slot >= blockHead.numSlots)
        {
            // set the block to the next block
            block = blockHead.rblock;
            //reset the slot to 0
            slot = 0;
            // continue to the next iteration
            continue;
        }
        // if the slot is unoccupied, move to the next slot
        if (slotMap[slot] == SLOT_UNOCCUPIED)
        {
            //increment the slot number
            slot++;
            //continue to the next iteration
            continue;
        }
        // get the attribute offset
        //we need to get the attribute offset of the attribute we are searching for
        //we use the attrCacheTable to get the attribute catalog entry of the attribute
        //we pass the relId and the attribute name to the getAttrCatEntry function
        //we use this attribute offset to get the attribute at the offset in the record
        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);
        //this uses a for loop to traverse the attrCacheTable's relid entry linked list 
        //to get the attribute catalog entry of the attribute
        //it compares each node of linked list's attrCatEntry's attrName with the attrName

        //we get the attribute offset from the attribute catalog entry
        int attrOffset = attrCatEntry.offset;
        // compare the attribute value with the value in the record and see if it satisfies the condition
        //this function will return if the attribute value is >, <, =, !=, >=, <= the value in the record
        //0=equal(a=b), 1=greater(a>b), -1=less(a<b)
        int cmpVal = compareAttrs(record[attrOffset]/*a*/, attrVal/*b*/, attrCatEntry.attrType);

        if (
            (op == NE && cmpVal != 0) ||
            (op == LT && cmpVal < 0) ||
            (op == LE && cmpVal <= 0) ||
            (op == EQ && cmpVal == 0) ||
            (op == GT && cmpVal > 0) ||
            (op == GE && cmpVal >= 0))
        {
            //set the search index to the current record
            //we set the search index to the current record so that we can start from the next record
            //in the next search
            RecId matchRecId{block, slot};
            RelCacheTable::setSearchIndex(relId, &matchRecId);
            //return the record id block and slot nmber
            return RecId{block, slot};
        }
        // move to the next slot
        slot++;
    }
    // if no record is found, return {-1, -1}
    return RecId{-1, -1};
}