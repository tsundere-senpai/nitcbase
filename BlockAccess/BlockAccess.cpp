#include "BlockAccess.h"
#include <stdio.h>
#include <cstring>
static int compCount = 0;
int BlockAccess::getCompCount()
{
    return compCount;
}
RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op)
{
    // get the search index for the relation
    RecId prevRecId;
    RelCacheTable::getSearchIndex(relId, &prevRecId);

    // printf("relid=%d searchIndex=%d %d\n",relId,prevRecId.block,prevRecId.slot);
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
        // printf("name=%s\n recordname=%s\n",record[attrOffset].sVal,attrVal.sVal);

        int cmpVal = compareAttrs(record[attrOffset] /*a*/, attrVal /*b*/, attrCatEntry.attrType);
        compCount++;
        // printf("value=%d\n",cmpVal);
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
            // printf("block=%d\nslot=%d\n",SearchIndex.block,SearchIndex.slot);
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

int BlockAccess::project(int relId, Attribute *record)
{
    // get the previous search index of the relation relId from the relation
    // cache (use RelCacheTable::getSearchIndex() function)

    // declare block and slot which will be used to store the record id of the
    // slot we need to check.
    int block, slot;
    RecId prevRecId;
    RelCacheTable::getSearchIndex(relId, &prevRecId);

    /* if the current search index record is invalid(i.e. = {-1, -1})
       (this only happens when the caller reset the search index)
    */
    if (prevRecId.block == -1 && prevRecId.slot == -1)
    {
        // (new project operation. start from beginning)

        // get the first record block of the relation from the relation cache
        // (use RelCacheTable::getRelCatEntry() function of Cache Layer)
        RelCatEntry firstEntry;
        RelCacheTable::getRelCatEntry(relId, &firstEntry);
        block = firstEntry.firstBlk;
        slot = 0;
        // block = first record block of the relation
        // slot = 0
    }
    else
    {
        // (a project/search operation is already in progress)
        block = prevRecId.block;
        slot = prevRecId.slot + 1;
        // block = previous search index's block
        // slot = previous search index's slot + 1
    }

    // The following code finds the next record of the relation
    /* Start from the record id (block, slot) and iterate over the remaining
       records of the relation */
    while (block != -1)
    {
        // create a RecBuffer object for block (using appropriate constructor!)
        RecBuffer recBuff(block);
        HeadInfo header;
        recBuff.getHeader(&header);
        unsigned char slotMap[header.numSlots];
        recBuff.getSlotMap(slotMap);

        if (slot >= header.numSlots)
        {
            // (no more slots in this block)
            // update block = right block of block
            // update slot = 0
            // (NOTE: if this is the last block, rblock would be -1. this would
            //        set block = -1 and fail the loop condition )
            block = header.rblock;
            slot = 0;
        }
        else if (slotMap[slot] == SLOT_UNOCCUPIED)
        { // (i.e slot-th entry in slotMap contains SLOT_UNOCCUPIED)
            slot++;
            // increment slot
        }
        else
        {
            // (the next occupied slot / record has been found)
            break;
        }
    }

    if (block == -1)
    {
        // (a record was not found. all records exhausted)
        return E_NOTFOUND;
    }

    // declare nextRecId to store the RecId of the record found
    RecId nextRecId{block, slot};

    // set the search index to nextRecId using RelCacheTable::setSearchIndex
    RelCacheTable::setSearchIndex(relId, &nextRecId);

    /* Copy the record with record id (nextRecId) to the record buffer (record)
       For this Instantiate a RecBuffer class object by passing the recId and
       call the appropriate method to fetch the record
    */
    RecBuffer recordBuff(block);
    recordBuff.getRecord(record, slot);

    return SUCCESS;
}
int BlockAccess::insert(int relId, Attribute *record)
{
    // get the relation catalog entry from relation cache
    // ( use RelCacheTable::getRelCatEntry() of Cache Layer)
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId, &relCatEntry);

    int blockNum = relCatEntry.firstBlk;

    // rec_id will be used to store where the new record will be inserted
    RecId rec_id = {-1, -1};

    int numOfSlots = relCatEntry.numSlotsPerBlk;
    int numOfAttributes = relCatEntry.numAttrs;

    // block number of the last element in the linked list = -1
    int prevBlockNum = -1;

    // Traversing the linked list of existing record blocks of the relation
    // until a free slot is found OR until the end of the list is reached

    while (blockNum != -1)
    {
        // create a RecBuffer object for blockNum (using appropriate constructor!)
        RecBuffer blockBuffer(blockNum);

        // get header of block(blockNum) using RecBuffer::getHeader() function
        HeadInfo blockHeader;
        blockBuffer.getHeader(&blockHeader);

        // get slot map of block(blockNum) using RecBuffer::getSlotMap() function
        int numSlots = blockHeader.numSlots;
        unsigned char slotMap[numSlots];
        blockBuffer.getSlotMap(slotMap);

        // search for free slot in the block 'blockNum' and store it's rec-id in rec_id
        // (Free slot can be found by iterating over the slot map of the block)
        int slotIndex = 0;
        for (; slotIndex < numSlots; slotIndex++)
        {
            // if a free slot is found, set rec_id and discontinue the traversal
            // of the linked list of record blocks (break from the loop)
            //* slot map stores SLOT_UNOCCUPIED if slot is free and SLOT_OCCUPIED if slot is occupied
            if (slotMap[slotIndex] == SLOT_UNOCCUPIED)
            {
                rec_id = RecId{blockNum, slotIndex};
                break;
            }
        }

        if (rec_id.block != -1 || rec_id.slot != -1)
            break;

        /* otherwise, continue to check the next block by updating the
           block numbers as follows:
              update prevBlockNum = blockNum
              update blockNum = header.rblock (next element in the linked list of record blocks)
        */
        prevBlockNum = blockNum;
        blockNum = blockHeader.rblock;
    }

    //  if no free slot is found in existing record blocks (rec_id = {-1, -1})
    if (rec_id.block == -1 && rec_id.slot == -1)
    {
        // if relation is RELCAT, do not allocate any more blocks
        //     return E_MAXRELATIONS;
        if (relId == RELCAT_RELID)
            return E_MAXRELATIONS;

        // Otherwise,
        // get a new record block (using the appropriate RecBuffer constructor!)
        // get the block number of the newly allocated block
        // (use BlockBuffer::getBlockNum() function)
        RecBuffer blockBuffer;
        blockNum = blockBuffer.getBlockNum();

        // let ret be the return value of getBlockNum() function call
        if (blockNum == E_DISKFULL)
            return E_DISKFULL;

        // Assign rec_id.block = new block number(i.e. ret) and rec_id.slot = 0
        rec_id = RecId{blockNum, 0};

        HeadInfo blockHeader;
        blockHeader.blockType = REC;
        blockHeader.lblock = prevBlockNum, blockHeader.rblock = blockHeader.pblock = -1;
        blockHeader.numAttrs = numOfAttributes, blockHeader.numSlots = numOfSlots, blockHeader.numEntries = 0;

        blockBuffer.setHeader(&blockHeader);

        unsigned char slotMap[numOfSlots];
        for (int slotIndex = 0; slotIndex < numOfSlots; slotIndex++)
            slotMap[slotIndex] = SLOT_UNOCCUPIED;

        blockBuffer.setSlotMap(slotMap);

        // if prevBlockNum != -1
        if (prevBlockNum != -1)
        {
            // TODO: create a RecBuffer object for prevBlockNum
            RecBuffer prevBlockBuffer(prevBlockNum);

            // TODO: get the header of the block prevBlockNum and
            HeadInfo prevBlockHeader;
            prevBlockBuffer.getHeader(&prevBlockHeader);

            // TODO: update the rblock field of the header to the new block
            prevBlockHeader.rblock = blockNum;
            // number i.e. rec_id.block
            // (use BlockBuffer::setHeader() function)
            prevBlockBuffer.setHeader(&prevBlockHeader);
        }
        else
        {
            // update first block field in the relation catalog entry to the
            // new block (using RelCacheTable::setRelCatEntry() function)
            relCatEntry.firstBlk = blockNum;
            RelCacheTable::setRelCatEntry(relId, &relCatEntry);
        }

        // update last block field in the relation catalog entry to the
        // new block (using RelCacheTable::setRelCatEntry() function)
        relCatEntry.lastBlk = blockNum;
        RelCacheTable::setRelCatEntry(relId, &relCatEntry);
    }

    // create a RecBuffer object for rec_id.block
    RecBuffer blockBuffer(rec_id.block);

    // insert the record into rec_id'th slot using RecBuffer.setRecord())
    blockBuffer.setRecord(record, rec_id.slot);

    /* update the slot map of the block by marking entry of the slot to
       which record was inserted as occupied) */
    // (ie store SLOT_OCCUPIED in free_slot'th entry of slot map)
    // (use RecBuffer::getSlotMap() and RecBuffer::setSlotMap() functions)
    unsigned char slotmap[numOfSlots];
    blockBuffer.getSlotMap(slotmap);

    slotmap[rec_id.slot] = SLOT_OCCUPIED;
    blockBuffer.setSlotMap(slotmap);

    // increment the numEntries field in the header of the block to
    // which record was inserted
    // (use BlockBuffer::getHeader() and BlockBuffer::setHeader() functions)
    HeadInfo blockHeader;
    blockBuffer.getHeader(&blockHeader);

    blockHeader.numEntries++;
    blockBuffer.setHeader(&blockHeader);

    // Increment the number of records field in the relation cache entry for
    // the relation. (use RelCacheTable::setRelCatEntry function)
    relCatEntry.numRecs++;
    RelCacheTable::setRelCatEntry(relId, &relCatEntry);

    return SUCCESS;
}
/*
NOTE: This function will copy the result of the search to the `record` argument.
      The caller should ensure that space is allocated for `record` array
      based on the number of attributes in the relation.
*/
int BlockAccess::search(int relId, Attribute *record, char attrName[ATTR_SIZE], Attribute attrVal, int op)
{
    // Declare a variable called recid to store the searched record
    RecId recId;
    AttrCatEntry attrCatEntry;
    int ret = AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);
    if (ret != SUCCESS)
    {
        return ret;
    }
    int rootBlock = attrCatEntry.rootBlock;
    if (rootBlock == -1)
    {
        recId = linearSearch(relId, attrName, attrVal, op);
    }
    else
    {
        // printf("b+ search is happening\n");
        recId = BPlusTree::bPlusSearch(relId, attrName, attrVal, op);
    }

    /* search for the record id (recid) corresponding to the attribute with
    attribute name attrName, with value attrval and satisfying the condition op
    using linearSearch() */

    // if there's no record satisfying the given condition (recId = {-1, -1})
    //    return E_NOTFOUND;
    if (recId.block == -1 && recId.slot == -1)
    {
        // printf("unsuccessful search\n");
        return E_NOTFOUND;
    }
    // printf("%dblock %dslot \n",recId.block,recId.slot);
    /* Copy the record with record id (recId) to the record buffer (record)
       For this Instantiate a RecBuffer class object using recId and
       call the appropriate method to fetch the record
    */
    RecBuffer recBlock(recId.block);
    recBlock.getRecord(record, recId.slot);
    return SUCCESS;
}
int BlockAccess::deleteRelation(char relName[ATTR_SIZE])
{
    // if the relation to delete is either Relation Catalog or Attribute Catalog,
    //     return E_NOTPERMITTED
    // (check if the relation names are either "RELATIONCAT" and "ATTRIBUTECAT".
    // you may use the following constants: RELCAT_NAME and ATTRCAT_NAME)
    if (strcmp(RELCAT_RELNAME, relName) == 0 ||
        strcmp(ATTRCAT_RELNAME, relName) == 0)
    {
        return E_NOTPERMITTED;
    }

    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */

    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute relNameAttr; // (stores relName as type union Attribute)
    // assign relNameAttr.sVal = relName
    strcpy(relNameAttr.sVal, relName);
    //  linearSearch on the relation catalog for RelName = relNameAttr
    RecId recId = BlockAccess::linearSearch(RELCAT_RELID, RELCAT_ATTR_RELNAME, relNameAttr, EQ);
    // if the relation does not exist (linearSearch returned {-1, -1})
    //     return E_RELNOTEXIST
    if (recId.block == -1 && recId.slot == -1)
    {
        return E_RELNOTEXIST;
    }

    Attribute relCatEntryRecord[RELCAT_NO_ATTRS];
    /* store the relation catalog record corresponding to the relation in
       relCatEntryRecord using RecBuffer.getRecord */
    RecBuffer relCatBlock(recId.block);
    relCatBlock.getRecord(relCatEntryRecord, recId.slot);
    /* get the first record block of the relation (firstBlock) using the
       relation catalog entry record */
    int firstBlk = relCatEntryRecord[RELCAT_FIRST_BLOCK_INDEX].nVal;
    /* get the number of attributes corresponding to the relation (numAttrs)
       using the relation catalog entry record */
    int numAttrs = relCatEntryRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal;

    /*
     Delete all the record blocks of the relation
    */
    // for each record block of the relation:
    //     get block header using BlockBuffer.getHeader
    //     get the next block from the header (rblock)
    //     release the block using BlockBuffer.releaseBlock
    //
    //     Hint: to know if we reached the end, check if nextBlock = -1

    int nextBlock = firstBlk;
    while (nextBlock != -1)
    {
        RecBuffer block(nextBlock);
        HeadInfo header;
        block.getHeader(&header);
        nextBlock = header.rblock;
        block.releaseBlock();
    }

    /***
        Deleting attribute catalog entries corresponding the relation and index
        blocks corresponding to the relation with relName on its attributes
    ***/

    // reset the searchIndex of the attribute catalog
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    int numberOfAttributesDeleted = 0;

    while (true)
    {
        RecId attrCatRecId;
        // attrCatRecId = linearSearch on attribute catalog for RelName = relNameAttr
        attrCatRecId = linearSearch(ATTRCAT_RELID, ATTRCAT_ATTR_RELNAME, relNameAttr, EQ);
        // if no more attributes to iterate over (attrCatRecId == {-1, -1})
        //     break;
        if (attrCatRecId.block == -1 && attrCatRecId.slot == -1)
        {
            break;
        }

        numberOfAttributesDeleted++;

        // create a RecBuffer for attrCatRecId.block
        // get the header of the block
        // get the record corresponding to attrCatRecId.slot
        Attribute attrCatRec[ATTRCAT_NO_ATTRS];
        RecBuffer attrCatBuffer(attrCatRecId.block);
        attrCatBuffer.getRecord(attrCatRec, attrCatRecId.slot);

        // declare variable rootBlock which will be used to store the root
        // block field from the attribute catalog record.
        // int rootBlock = attrCatRec[ATTRCAT_ROOT_BLOCK_INDEX].nVal;
        // (This will be used later to delete any indexes if it exists)

        // Update the Slotmap for the block by setting the slot as SLOT_UNOCCUPIED
        // Hint: use RecBuffer.getSlotMap and RecBuffer.setSlotMap
        HeadInfo attrHeader;
        attrCatBuffer.getHeader(&attrHeader);
        unsigned char slotMap[attrHeader.numSlots];
        attrCatBuffer.getSlotMap(slotMap);
        slotMap[attrCatRecId.slot] = SLOT_UNOCCUPIED;
        attrCatBuffer.setSlotMap(slotMap);

        /* Decrement the numEntries in the header of the block corresponding to
           the attribute catalog entry and then set back the header
           using RecBuffer.setHeader */
        attrHeader.numEntries--;
        attrCatBuffer.setHeader(&attrHeader);
        /* If number of entries become 0, releaseBlock is called after fixing
           the linked list.
        */
        // if (/*   header.numEntries == 0  */)
        if (attrHeader.numEntries == 0)
        {
            /*
               Standard Double Linked List Delete for a Block
               Get the header of the left block and set it's rblock to this
               block's rblock

            */

            // create a RecBuffer for lblock and call appropriate methods
            RecBuffer lBlock(attrHeader.lblock);
            HeadInfo lHeader;
            lBlock.getHeader(&lHeader);
            lHeader.rblock = attrHeader.rblock;
            lBlock.setHeader(&lHeader);

            // if (/* header.rblock != -1 */)
            if (attrHeader.rblock != INVALID_BLOCKNUM)
            {
                /* Get the header of the right block and set it's lblock to
                   this block's lblock */
                // create a RecBuffer for rblock and call appropriate methods

                RecBuffer rBlock(attrHeader.rblock);
                HeadInfo rHeader;
                rBlock.getHeader(&rHeader);
                rHeader.lblock = attrHeader.lblock;
                rBlock.setHeader(&rHeader);
            }
            else
            {
                // (the block being released is the "Last Block" of the relation.)
                /* update the Relation Catalog entry's LastBlock field for this
                   relation with the block number of the previous block. */
                RelCatEntry relCatEntry;
                RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &relCatEntry);
                relCatEntry.lastBlk = attrHeader.lblock;
                RelCacheTable::setRelCatEntry(ATTRCAT_RELID, &relCatEntry);
            }

            // (Since the attribute catalog will never be empty(why?), we do not
            //  need to handle the case of the linked list becoming empty - i.e
            //  every block of the attribute catalog gets released.)

            // call releaseBlock()
            attrCatBuffer.releaseBlock();
        }

        // (the following part is only relevant once indexing has been implemented)
        // if index exists for the attribute (rootBlock != -1), call bplus destroy
        /*
           if (rootBlock != -1)
            {
                // delete the bplus tree rooted at rootBlock using BPlusTree::bPlusDestroy()
            }
                */
    }

    /*** Delete the entry corresponding to the relation from relation catalog ***/
    // Fetch the header of Relcat block
    HeadInfo relHeader;
    relCatBlock.getHeader(&relHeader);
    relHeader.numEntries--;
    relCatBlock.setHeader(&relHeader);
    /* Decrement the numEntries in the header of the block corresponding to the
       relation catalog entry and set it back */

    /* Get the slotmap in relation catalog, update it by marking the slot as
       free(SLOT_UNOCCUPIED) and set it back. */

    unsigned char relSlotMap[relHeader.numSlots];
    relCatBlock.getSlotMap(relSlotMap);
    relSlotMap[recId.slot] = SLOT_UNOCCUPIED;
    relCatBlock.setSlotMap(relSlotMap);

    /*** Updating the Relation Cache Table ***/
    /** Update relation catalog record entry (number of records in relation
        catalog is decreased by 1) **/
    // Get the entry corresponding to relation catalog from the relation
    // cache and update the number of records and set it back
    // (using RelCacheTable::setRelCatEntry() function)
    RelCatEntry relEntry;
    RelCacheTable::getRelCatEntry(RELCAT_RELID, &relEntry);
    relEntry.numRecs--;
    RelCacheTable::setRelCatEntry(RELCAT_RELID, &relEntry);

    /** Update attribute catalog entry (number of records in attribute catalog
        is decreased by numberOfAttributesDeleted) **/
    // i.e., #Records = #Records - numberOfAttributesDeleted
    RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &relEntry);
    relEntry.numRecs -= numberOfAttributesDeleted;
    RelCacheTable::setRelCatEntry(ATTRCAT_RELID, &relEntry);

    // Get the entry corresponding to attribute catalog from the relation
    // cache and update the number of records and set it back
    // (using RelCacheTable::setRelCatEntry() function)

    return SUCCESS;
}