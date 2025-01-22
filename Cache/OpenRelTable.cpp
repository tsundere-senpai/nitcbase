#include "OpenRelTable.h"
#include <cstring>
#include <stdlib.h>


AttrCacheEntry* createAttrCacheEntryList(int size) {
    AttrCacheEntry *head = nullptr, *curr = nullptr;
    head = curr = (AttrCacheEntry*) malloc(sizeof(AttrCacheEntry));
    size--;
    while (size--) {
        curr->next = (AttrCacheEntry*) malloc(sizeof(AttrCacheEntry));
        curr = curr->next;
    }
    curr->next = nullptr;
    return head;
}

OpenRelTable::OpenRelTable()
{
    // initialize relCache and attrCache with nullptr
    for (int i = 0; i < MAX_OPEN; ++i) {
        RelCacheTable::relCache[i] = nullptr;
        AttrCacheTable::attrCache[i] = nullptr;
    }

    /************ Setting up Relation Cache entries ************/
    // setting up the variables
    RecBuffer relCatBlock(RELCAT_BLOCK);
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    RelCacheEntry *relCacheEntry = nullptr;

    // Set up Relation Catalog in RelCache
    relCatBlock.getRecord(relCatRecord, RELCAT_RELID);
    relCacheEntry = (RelCacheEntry*) malloc(sizeof(RelCacheEntry));
    RelCacheTable::recordToRelCatEntry(relCatRecord, &(relCacheEntry->relCatEntry));
    relCacheEntry->recId.block = RELCAT_BLOCK;
    relCacheEntry->recId.slot = RELCAT_SLOTNUM_FOR_RELCAT;
    RelCacheTable::relCache[RELCAT_RELID] = relCacheEntry;

    // Set up Attribute Catalog in RelCache
    relCatBlock.getRecord(relCatRecord, ATTRCAT_RELID);
    relCacheEntry = (RelCacheEntry*) malloc(sizeof(RelCacheEntry));
    RelCacheTable::recordToRelCatEntry(relCatRecord, &(relCacheEntry->relCatEntry));
    relCacheEntry->recId.block = RELCAT_BLOCK;
    relCacheEntry->recId.slot = ATTRCAT_RELID;
    RelCacheTable::relCache[ATTRCAT_RELID] = relCacheEntry;

    /************ Setting up Attribute cache entries ************/
    RecBuffer attrCatBlock(ATTRCAT_BLOCK);
    Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
    AttrCacheEntry *attrCacheEntry = nullptr, *head = nullptr;
    
    /**** setting up Relation Catalog relation in the Attribute Cache Table ****/
    int numAttrsRelCat = RelCacheTable::relCache[RELCAT_RELID]->relCatEntry.numAttrs;
    head = createAttrCacheEntryList(numAttrsRelCat);
    attrCacheEntry = head;

    // Process slots 0 to 5 for Relation Catalog attributes
    for(int slotNum = 0; slotNum < numAttrsRelCat; slotNum++) {
        attrCatBlock.getRecord(attrCatRecord, slotNum);
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &(attrCacheEntry->attrCatEntry));
        attrCacheEntry->recId.block = ATTRCAT_BLOCK;
        attrCacheEntry->recId.slot = slotNum;
        attrCacheEntry = attrCacheEntry->next;
    }
    AttrCacheTable::attrCache[RELCAT_RELID] = head;
    
    /**** setting up Attribute Catalog relation in the Attribute Cache Table ****/
    int numAttrsAttrCat = RelCacheTable::relCache[ATTRCAT_RELID]->relCatEntry.numAttrs;
    head = createAttrCacheEntryList(numAttrsAttrCat);
    attrCacheEntry = head;

    // Process slots 6-11 for Attribute Catalog attributes
    for(int slotNum = numAttrsRelCat; slotNum < numAttrsRelCat + numAttrsAttrCat; slotNum++) {
        attrCatBlock.getRecord(attrCatRecord, slotNum);
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &(attrCacheEntry->attrCatEntry));
        attrCacheEntry->recId.block = ATTRCAT_BLOCK;
        attrCacheEntry->recId.slot = slotNum;
        attrCacheEntry = attrCacheEntry->next;
    }
    AttrCacheTable::attrCache[ATTRCAT_RELID] = head;
}

OpenRelTable::~OpenRelTable()
{
    
    // free all the memory that you allocated in the constructor
}