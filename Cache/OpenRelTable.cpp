#include "OpenRelTable.h"
#include <cstring>
#include <stdlib.h>
#include <stdio.h>
OpenRelTableMetaInfo OpenRelTable::tableMetaInfo[MAX_OPEN];

AttrCacheEntry *createAttrCacheEntryList(int size)
{
    AttrCacheEntry *head = nullptr, *curr = nullptr;
    head = curr = (AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
    size--;
    while (size--)
    {
        curr->next = (AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
        curr = curr->next;
    }
    curr->next = nullptr;
    return head;
}

OpenRelTable::OpenRelTable()
{
    // initialize relCache and attrCache with nullptr
    for (int i = 0; i < MAX_OPEN; ++i)
    {
        RelCacheTable::relCache[i] = nullptr;
        AttrCacheTable::attrCache[i] = nullptr;
        tableMetaInfo[i].free = true;
    }

    /************ Setting up Relation Cache entries ************/
    // setting up the variables
    RecBuffer relCatBlock(RELCAT_BLOCK);
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    RelCacheEntry *relCacheEntry = nullptr;
    int slotNum = 0;

    for (int relId = RELCAT_RELID; relId <= ATTRCAT_RELID; relId++)
    {
        relCatBlock.getRecord(relCatRecord, relId);
        relCacheEntry = (RelCacheEntry *)malloc(sizeof(RelCacheEntry));
        RelCacheTable::recordToRelCatEntry(relCatRecord, &(relCacheEntry->relCatEntry));
        relCacheEntry->recId.block = RELCAT_BLOCK;
        relCacheEntry->recId.slot = relId;
        RelCacheTable::relCache[relId] = relCacheEntry;
    }
    /************ Setting up Attribute cache entries ************/
    RecBuffer attrCatBlock(ATTRCAT_BLOCK);
    Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
    AttrCacheEntry *attrCacheEntry = nullptr, *head = nullptr;
    int slot = 0;
    for (int relId = RELCAT_RELID, recordId = 0; relId <= ATTRCAT_RELID; relId++)
    {
        int numAttrs = RelCacheTable::relCache[relId]->relCatEntry.numAttrs;
        head = createAttrCacheEntryList(numAttrs);
        attrCacheEntry = head;

        while (numAttrs--)
        {
            attrCatBlock.getRecord(attrCatRecord, recordId);
            AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &(attrCacheEntry->attrCatEntry));
            attrCacheEntry->recId.block = ATTRCAT_BLOCK;
            attrCacheEntry->recId.slot = recordId++;
            attrCacheEntry = attrCacheEntry->next;
        }
        AttrCacheTable::attrCache[relId] = head;
    }
    tableMetaInfo[RELCAT_RELID].free = false;
    strcpy(tableMetaInfo[RELCAT_RELID].relName, RELCAT_RELNAME);
    tableMetaInfo[ATTRCAT_RELID].free = false;
    strcpy(tableMetaInfo[ATTRCAT_RELID].relName, ATTRCAT_RELNAME);
}

int OpenRelTable::getRelId(char relName[ATTR_SIZE])
{
    for (int i = 0; i < MAX_OPEN; i++)
    {
        if (strcmp(tableMetaInfo[i].relName, relName) == 0 && !tableMetaInfo[i].free)
        {
            return i;
        }
    }
    return E_RELNOTOPEN;
}
int OpenRelTable::openRel(char relName[ATTR_SIZE])
{

    int temp = OpenRelTable::getRelId(relName);
    if (temp != E_RELNOTOPEN)
    {

        return temp;
    }

    int relId = getFreeOpenRelTableEntry();
    if (relId == E_CACHEFULL)
    {
        return E_CACHEFULL;
    }

    Attribute attrval;
    strcpy(attrval.sVal, relName);
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    RecId relcatRecId = BlockAccess::linearSearch(RELCAT_RELID, RELCAT_ATTR_RELNAME, attrval, EQ);

    if (relcatRecId.block == -1 && relcatRecId.slot == -1)
    {

        return E_RELNOTEXIST;
    }

    RecBuffer relBuffer(relcatRecId.block);
    Attribute record[RELCAT_NO_ATTRS];
    RelCacheEntry *relCacheBuffer = nullptr;

    relBuffer.getRecord(record, relcatRecId.slot);
    relCacheBuffer = (RelCacheEntry *)malloc(sizeof(RelCacheEntry));
    RelCacheTable::recordToRelCatEntry(record, &(relCacheBuffer->relCatEntry));
    relCacheBuffer->recId.block = relcatRecId.block;
    relCacheBuffer->recId.slot = relcatRecId.slot;

    RelCacheTable::relCache[relId] = relCacheBuffer;

    Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
    AttrCacheEntry *attrCacheEntry = nullptr;
    AttrCacheEntry *listHead = nullptr;
    int numberOfAttributes = RelCacheTable::relCache[relId]->relCatEntry.numAttrs;
    listHead = createAttrCacheEntryList(numberOfAttributes);
    attrCacheEntry = listHead;

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    for (int attr = 0; attr < numberOfAttributes; attr++)
    {

        RecId attrcatRecId = BlockAccess::linearSearch(ATTRCAT_RELID, RELCAT_ATTR_RELNAME, attrval, EQ);

        RecBuffer attrCatBuffer(attrcatRecId.block);
        attrCatBuffer.getRecord(attrCatRecord, attrcatRecId.slot);

        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &(attrCacheEntry->attrCatEntry));
        attrCacheEntry->recId.block = attrcatRecId.block;
        attrCacheEntry->recId.slot = attrcatRecId.slot;
        attrCacheEntry = attrCacheEntry->next;
    }
    AttrCacheTable::attrCache[relId] = listHead;

    tableMetaInfo[relId].free = false;
    strcpy(tableMetaInfo[relId].relName, relName);

    return relId;
}

int OpenRelTable::closeRel(int relId)
{
    if (relId == RELCAT_RELID || relId == ATTRCAT_RELID)
    {
        return E_NOTPERMITTED;
    }

    if (relId < 0 || relId >= MAX_OPEN)
    {
        return E_OUTOFBOUND;
    }

    if (tableMetaInfo[relId].free)
    {
        return E_RELNOTOPEN;
    }
    if (RelCacheTable::relCache[relId]->dirty)
    {

        /* Get the Relation Catalog entry from RelCacheTable::relCache
        Then convert it to a record using RelCacheTable::relCatEntryToRecord(). */
        Attribute Record[RELCAT_NO_ATTRS];
        RelCacheTable::relCatEntryToRecord(&(RelCacheTable::relCache[relId]->relCatEntry), Record);
        RecId recId = RelCacheTable::relCache[relId]->recId;
        // declaring an object of RecBuffer class to write back to the buffer
        RecBuffer relCatBlock(recId.block);

        // Write back to the buffer using relCatBlock.setRecord() with recId.slot
        relCatBlock.setRecord(Record, recId.slot);
    }
    free(RelCacheTable::relCache[relId]);
    AttrCacheEntry *head = AttrCacheTable::attrCache[relId];
    AttrCacheEntry *next = head->next;
    while (next != nullptr)
    {
        free(head);
        head = next;
        next = next->next;
    }
    free(head);
    // update `tableMetaInfo` to set `relId` as a free slot
    // update `relCache` and `attrCache` to set the entry at `relId` to nullptr
    tableMetaInfo[relId].free = true;
    RelCacheTable::relCache[relId] = nullptr;
    AttrCacheTable::attrCache[relId] = nullptr;

    return SUCCESS;
}

int OpenRelTable::getFreeOpenRelTableEntry()
{

    /* traverse through the tableMetaInfo array,
      find a free entry in the Open Relation Table.*/
    for (int i = 2; i < MAX_OPEN; i++)
    {
        if (tableMetaInfo[i].free)
        {
            return i;
        }
    }
    return E_CACHEFULL;

    // if found return the relation id, else return E_CACHEFULL.
}

OpenRelTable::~OpenRelTable()
{
    // close all open relations (from rel-id = 2 onwards. Why?)
    for (int i = 2; i < MAX_OPEN; ++i)
    {
        if (!tableMetaInfo[i].free)
        {
            OpenRelTable::closeRel(i); // we will implement this function later
        }
    }
    if (RelCacheTable::relCache[RELCAT_RELID]->dirty)
    {
        Attribute relCatRecord[RELCAT_NO_ATTRS];
        RelCacheTable::relCatEntryToRecord(&(RelCacheTable::relCache[RELCAT_RELID]->relCatEntry), relCatRecord);
        RecBuffer relCatBlock(RelCacheTable::relCache[RELCAT_RELID]->recId.block);
        relCatBlock.setRecord(relCatRecord, RelCacheTable::relCache[RELCAT_RELID]->recId.slot);
    }

    free(RelCacheTable::relCache[RELCAT_RELID]);

    if (RelCacheTable::relCache[ATTRCAT_RELID]->dirty)
    {
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        RelCacheTable::relCatEntryToRecord(&(RelCacheTable::relCache[ATTRCAT_RELID]->relCatEntry), attrCatRecord);
        RecBuffer attrCatBlock(RelCacheTable::relCache[ATTRCAT_RELID]->recId.block);
        attrCatBlock.setRecord(attrCatRecord, RelCacheTable::relCache[ATTRCAT_RELID]->recId.slot);
    }
    free(RelCacheTable::relCache[ATTRCAT_RELID]);

    for (int relId = ATTRCAT_RELID; relId >= RELCAT_RELID; relId--)
	{
		AttrCacheEntry *curr = AttrCacheTable::attrCache[relId], *next = nullptr;
		for (int attrIndex = 0; attrIndex < 6; attrIndex++)
		{
			next = curr->next;

			// check if the AttrCatEntry was written back
			if (curr->dirty)
			{
				AttrCatEntry attrCatBuffer;
				AttrCacheTable::getAttrCatEntry(relId, attrIndex, &attrCatBuffer);

				Attribute attrCatRecord [ATTRCAT_NO_ATTRS];
				AttrCacheTable::attrCatEntryToRecord(&attrCatBuffer, attrCatRecord);

				RecId recId = curr->recId;

				// declaring an object if RecBuffer class to write back to the buffer
				RecBuffer attrCatBlock (recId.block);

				// write back to the buffer using RecBufer.setRecord()
				attrCatBlock.setRecord(attrCatRecord, recId.slot);
			}

			free (curr);
			curr = next;
		}
	}
    // free all the memory that you allocated in the constructor
}