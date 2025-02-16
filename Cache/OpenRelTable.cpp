#include "OpenRelTable.h"
#include <cstring>
#include <stdlib.h>

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
/* This function will open a relation having name `relName`.
Since we are currently only working with the relation and attribute catalog, we
will just hardcode it. In subsequent stages, we will loop through all the relations
and open the appropriate one.
*/
int OpenRelTable::getRelId(char relName[ATTR_SIZE])
{

    // if relname is RELCAT_RELNAME, return RELCAT_RELID
    // if relname is ATTRCAT_RELNAME, return ATTRCAT_RELID

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
        // (checked using OpenRelTable::getRelId())
        return temp;
        // return that relation id;
    }

    /* find a free slot in the Open Relation Table
       using OpenRelTable::getFreeOpenRelTableEntry(). */

    int relId = getFreeOpenRelTableEntry();
    if (relId == E_CACHEFULL)
    {
        return E_CACHEFULL;
    }

    // let relId be used to store the free slot.

    /****** Setting up Relation Cache entry for the relation ******/

    /* search for the entry with relation name, relName, in the Relation Catalog using
        BlockAccess::linearSearch().
        Care should be taken to reset the searchIndex of the relation RELCAT_RELID
        before calling linearSearch().*/
    Attribute attrval;
    strcpy(attrval.sVal, relName);
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    RecId relcatRecId = BlockAccess::linearSearch(RELCAT_RELID, RELCAT_ATTR_RELNAME, attrval, EQ);
    // relcatRecId stores the rec-id of the relation `relName` in the Relation Catalog.

    if (relcatRecId.block == -1 && relcatRecId.slot == -1)
    {
        // (the relation is not found in the Relation Catalog.)
        return E_RELNOTEXIST;
    }

    /* read the record entry corresponding to relcatRecId and create a relCacheEntry
        on it using RecBuffer::getRecord() and RelCacheTable::recordToRelCatEntry().
        update the recId field of this Relation Cache entry to relcatRecId.
        use the Relation Cache entry to set the relId-th entry of the RelCacheTable.
      NOTE: make sure to allocate memory for the RelCacheEntry using malloc()
    */
    RecBuffer relBuffer(relcatRecId.block);
    Attribute record[RELCAT_NO_ATTRS];
    RelCacheEntry *relCacheBuffer = nullptr;

    relBuffer.getRecord(record, relcatRecId.slot);
    relCacheBuffer = (RelCacheEntry *)malloc(sizeof(RelCacheEntry));
    RelCacheTable::recordToRelCatEntry(record, &(relCacheBuffer->relCatEntry));
    relCacheBuffer->recId.block = relcatRecId.block;
    relCacheBuffer->recId.slot = relcatRecId.slot;

    RelCacheTable::relCache[relId] = relCacheBuffer;

    /****** Setting up Attribute Cache entry for the relation ******/

    // let listHead be used to hold the head of the linked list of attrCache entries.

    /*iterate over all the entries in the Attribute Catalog corresponding to each
    attribute of the relation relName by multiple calls of BlockAccess::linearSearch()
    care should be taken to reset the searchIndex of the relation, ATTRCAT_RELID,
    corresponding to Attribute Catalog before the first call to linearSearch().*/
    Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
    AttrCacheEntry *attrCacheEntry = nullptr;
    AttrCacheEntry *listHead=nullptr;
    int numberOfAttributes = RelCacheTable::relCache[relId]->relCatEntry.numAttrs;
    listHead = createAttrCacheEntryList(numberOfAttributes);
    attrCacheEntry = listHead;

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    for(int attr=0;attr<numberOfAttributes;attr++)
    {
        /* let attrcatRecId store a valid record id an entry of the relation, relName,
        in the Attribute Catalog.*/
        RecId attrcatRecId=BlockAccess::linearSearch(ATTRCAT_RELID,RELCAT_ATTR_RELNAME,attrval,EQ);

        RecBuffer attrCatBuffer(attrcatRecId.block);
        attrCatBuffer.getRecord(attrCatRecord,attrcatRecId.slot);

        AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&(attrCacheEntry->attrCatEntry));
        attrCacheEntry->recId.block=attrcatRecId.block;
        attrCacheEntry->recId.slot=attrcatRecId.slot;
        attrCacheEntry=attrCacheEntry->next;

        /* read the record entry corresponding to attrcatRecId and create an
        Attribute Cache entry on it using RecBuffer::getRecord() and
        AttrCacheTable::recordToAttrCatEntry().
        update the recId field of this Attribute Cache entry to attrcatRecId.
        add the Attribute Cache entry to the linked list of listHead .*/
        // NOTE: make sure to allocate memory for the AttrCacheEntry using malloc()
    }
    AttrCacheTable::attrCache[relId]=listHead;
    // set the relIdth entry of the AttrCacheTable to listHead.

    /****** Setting up metadata in the Open Relation Table for the relation******/

    // update the relIdth entry of the tableMetaInfo with free as false and
    // relName as the input.
    tableMetaInfo[relId].free=false;
    strcpy(tableMetaInfo[relId].relName,relName);


    return relId;
}

int OpenRelTable::closeRel(int relId) {
    if (relId == RELCAT_RELID || relId == ATTRCAT_RELID) {
      return E_NOTPERMITTED;
    }
  
    if (relId<0 || relId>=MAX_OPEN) {
      return E_OUTOFBOUND;
    }
  
    if (tableMetaInfo[relId].free) {
      return E_RELNOTOPEN;
    }
  
    // free the memory allocated in the relation and attribute caches which was
    // allocated in the OpenRelTable::openRel() function
    free(RelCacheTable::relCache[relId]);
    AttrCacheEntry *head=AttrCacheTable::attrCache[relId];
    AttrCacheEntry *next=head->next;
    while(next!=nullptr) {
      free(head);
      head=next;
      next=next->next;
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
    for (int relId = RELCAT_RELID; relId <= ATTRCAT_RELID; relId++)
    {
        // Free the RelCache entry
        if (RelCacheTable::relCache[relId] != nullptr)
        {
            free(RelCacheTable::relCache[relId]);
            RelCacheTable::relCache[relId] = nullptr;
        }

        // Free the linked list of AttrCache entries
        AttrCacheEntry *current = AttrCacheTable::attrCache[relId];
        while (current != nullptr)
        {
            AttrCacheEntry *next = current->next;
            free(current);
            current = next;
        }
        AttrCacheTable::attrCache[relId] = nullptr;
    }
    // free all the memory that you allocated in the constructor
}