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
        // we use the relCatEntry to get the first block of the relation
        // because the first block of the relation is stored in the relCatEntry

        RelCatEntry relCatEntry;
        RelCacheTable::getRelCatEntry(relId, &relCatEntry);
        block = relCatEntry.firstBlk;
        // start from the first slot
        slot = 0;
    }
    else // start from the next record
    {
        // set block to the block number of the previous record
        block = prevRecId.block;
        // set slot to the slot number of the previous record + 1 ie the next record (incrementing the slot number)
        slot = prevRecId.slot + 1;
    }
    // loop through all the blocks of the relation
    while (block != -1)
    {
        // get the block buffer
        // we initialise the blockBuffer with the block number
        // this is because we want to access the block
        RecBuffer blockBuffer(block);
        // get the header of the block
        HeadInfo blockHead;
        blockBuffer.getHeader(&blockHead);
        // get the record at the current slot
        // using headInfo.numAttrs to get the number of attributes in the block to set the size of the record array
        Attribute record[blockHead.numAttrs];
        // we already initialised the blockBuffer with the block number
        // using the blockBuffer to get the record at the current slot
        // we pass the record array and the slot number to the getRecord function
        // this function will fill the record array with the record at the slot number
        blockBuffer.getRecord(record, slot);

        // we can now get the slot map of the block
        unsigned char slotMap[blockHead.numSlots];
        // we use getslotmap from blockBuffer to get the slot map of the block
        // this function will fill the slotMap array with the slot map of the block
        // it will use loadBlockAndGetBufferPtr to get the buffer of the block
        // and then get the header of the block
        // it will get numofslots from header and then set a unsigned char pointer to the slot map in the buffer using the header size(32)
        // it will then get the slot map from the buffer and copy it to the slotMap array using a for loop
        blockBuffer.getSlotMap(slotMap);

        // if the slot is greater than the number of slots in the block, move to the next block
        if (slot >= blockHead.numSlots)
        {
            // set the block to the next block
            block = blockHead.rblock;
            // reset the slot to 0
            slot = 0;
            // continue to the next iteration
            continue;
        }
        // if the slot is unoccupied, move to the next slot
        if (slotMap[slot] == SLOT_UNOCCUPIED)
        {
            // increment the slot number
            slot++;
            // continue to the next iteration
            continue;
        }
        // get the attribute offset
        // we need to get the attribute offset of the attribute we are searching for
        // we use the attrCacheTable to get the attribute catalog entry of the attribute
        // we pass the relId and the attribute name to the getAttrCatEntry function
        // we use this attribute offset to get the attribute at the offset in the record
        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);
        // this uses a for loop to traverse the attrCacheTable's relid entry linked list
        // to get the attribute catalog entry of the attribute
        // it compares each node of linked list's attrCatEntry's attrName with the attrName

        // we get the attribute offset from the attribute catalog entry
        int attrOffset = attrCatEntry.offset;
        // compare the attribute value with the value in the record and see if it satisfies the condition
        // this function will return if the attribute value is >, <, =, !=, >=, <= the value in the record
        // 0=equal(a=b), 1=greater(a>b), -1=less(a<b)
        int cmpVal = compareAttrs(record[attrOffset] /*a*/, attrVal /*b*/, attrCatEntry.attrType);

        if (
            (op == NE && cmpVal != 0) ||
            (op == LT && cmpVal < 0) ||
            (op == LE && cmpVal <= 0) ||
            (op == EQ && cmpVal == 0) ||
            (op == GT && cmpVal > 0) ||
            (op == GE && cmpVal >= 0))
        {
            // set the search index to the current record
            // we set the search index to the current record so that we can start from the next record
            // in the next search
            RecId matchRecId{block, slot};
            RelCacheTable::setSearchIndex(relId, &matchRecId);
            // return the record id block and slot nmber
            return RecId{block, slot};
        }
        // move to the next slot
        slot++;
    }

    return RecId{-1, -1};
}
int BlockAccess::renameRelation(char oldName[ATTR_SIZE], char newName[ATTR_SIZE])
{
    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute newRelationName; // set newRelationName with newName
    strcpy(newRelationName.sVal, newName);
    // search the relation catalog for an entry with "RelName" = newRelationName
    RecId recId = linearSearch(RELCAT_RELID, RELCAT_ATTR_RELNAME, newRelationName, EQ);
    // If relation with name newName already exists (result of linearSearch
    //                                               is not {-1, -1})
    //    return E_RELEXIST;
    if (recId.block != -1 && recId.slot != -1)
    {
        return E_RELEXIST;
    }
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */

    Attribute oldRelationName; // set oldRelationName with oldName
    strcpy(oldRelationName.sVal, oldName);

    // search the relation catalog for an entry with "RelName" = oldRelationName
    RecId oldRecId = linearSearch(RELCAT_RELID, RELCAT_ATTR_RELNAME, oldRelationName, EQ);
    // If relation with name oldName does not exist (result of linearSearch is {-1, -1})
    //    return E_RELNOTEXIST;
    if (oldRecId.block == -1 && oldRecId.slot == -1)
    {
        return E_RELNOTEXIST;
    }
    /* get the relation catalog record of the relation to rename using a RecBuffer
       on the relation catalog [RELCAT_BLOCK] and RecBuffer.getRecord function
    */
    RecBuffer recBuffer(RELCAT_BLOCK);
    Attribute record[RELCAT_NO_ATTRS];
    recBuffer.getRecord(record, oldRecId.slot);
    /* update the relation name attribute in the record with newName.
       (use RELCAT_REL_NAME_INDEX) */
    strcpy(record[RELCAT_REL_NAME_INDEX].sVal, newName);
    // set back the record value using RecBuffer.setRecord
    recBuffer.setRecord(record, oldRecId.slot);
    /*
    update all the attribute catalog entries in the attribute catalog corresponding
    to the relation with relation name oldName to the relation name newName
    */

    /* reset the searchIndex of the attribute catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    // search the attribute catalog for an entry with "RelName" = oldRelationName
    int numberofAttributes = record[RELCAT_NO_ATTRIBUTES_INDEX].nVal;
    // for i = 0 to numberOfAttributes :
    //     linearSearch on the attribute catalog for relName = oldRelationName
    //     get the record using RecBuffer.getRecord
    //
    //     update the relName field in the record to newName
    //     set back the record using RecBuffer.setRecord
    for (int i = 0; i < numberofAttributes; i++)
    {
        RecId oldAttrRecId = linearSearch(ATTRCAT_RELID, ATTRCAT_ATTR_RELNAME, oldRelationName, EQ);
        RecBuffer attrBuffer(oldAttrRecId.block);
        Attribute attrRecord[ATTRCAT_NO_ATTRS];
        attrBuffer.getRecord(attrRecord, oldAttrRecId.slot);
        strcpy(attrRecord[ATTRCAT_REL_NAME_INDEX].sVal, newName);
        attrBuffer.setRecord(attrRecord, oldAttrRecId.slot);
    }
    return SUCCESS;
}
int BlockAccess::renameAttribute(char relName[ATTR_SIZE], char oldName[ATTR_SIZE], char newName[ATTR_SIZE])
{

    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute relNameAttr;
    strcpy(relNameAttr.sVal, relName);

    RecId SearchIndex = linearSearch(RELCAT_RELID, RELCAT_ATTR_RELNAME, relNameAttr, EQ);
    if (SearchIndex.block == -1 && SearchIndex.slot == -1)
    {
        return E_RELNOTEXIST;
    }

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    RecId attrToRenameRecId{-1, -1};
    Attribute attrCatEntryRecord[ATTRCAT_NO_ATTRS];

    while (true)
    {

        SearchIndex = linearSearch(ATTRCAT_RELID, ATTRCAT_ATTR_RELNAME, relNameAttr, EQ);

        if (SearchIndex.block == -1 && SearchIndex.slot == -1)
            break;

        RecBuffer attrCatBuff(SearchIndex.block);
        attrCatBuff.getRecord(attrCatEntryRecord, SearchIndex.slot);

        if (strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, oldName) == 0)
        {
            attrToRenameRecId.block = SearchIndex.block;
            attrToRenameRecId.slot = SearchIndex.slot;
            break;
        }

        if (strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName) == 0)
        {
            return E_ATTREXIST;
        }
    }
    if (attrToRenameRecId.slot == -1 && attrToRenameRecId.block == -1)
    {
        return E_ATTRNOTEXIST;
    }

    Attribute renameRecord[ATTRCAT_NO_ATTRS];
    RecBuffer renameBlock(attrToRenameRecId.block);
    renameBlock.getRecord(renameRecord, attrToRenameRecId.slot);
    strcpy(renameRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName);
    renameBlock.setRecord(renameRecord, attrToRenameRecId.slot);

    return SUCCESS;
}