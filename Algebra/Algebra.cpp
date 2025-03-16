#include "Algebra.h"

#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
/* used to select all the records that satisfy a condition.
the arguments of the function are
- srcRel - the source relation we want to select from
- targetRel - the relation we want to select into. (ignore for now)
- attr - the attribute that the condition is checking
- op - the operator of the condition
- strVal - the value that we want to compare against (represented as a string)
*/
int Algebra::select(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], char attr[ATTR_SIZE], int op, char strVal[ATTR_SIZE])
{

    // this part is for fetching the relid of the source relation
    int srcRelId = OpenRelTable::getRelId(srcRel); // we'll implement this later
    if (srcRelId == E_RELNOTOPEN)
    {
        return E_RELNOTOPEN;
    }
    // relid will be used throught the function to access the relation
    // we'll use the relid to get the first the attrcatentry to know the attrtype
    // this part compares the attribute type with the value type
    // copies the value to a attr variable sval for string and nval for number
    // we'll use the attr variable to compare with the attribute in the relation
    AttrCatEntry attrCatEntry;
    int ret = AttrCacheTable::getAttrCatEntry(srcRelId, attr, &attrCatEntry);
    // get the attribute catalog entry for attr, using AttrCacheTable::getAttrcatEntry()
    //    return E_ATTRNOTEXIST if it returns the error
    if (ret == E_ATTRNOTEXIST)
    {
        return E_ATTRNOTEXIST;
    }

    /*** Convert strVal (string) to an attribute of data type NUMBER or STRING ***/
    int type = attrCatEntry.attrType;
    Attribute attrVal;
    if (type == NUMBER)
    {
        if (isNumber(strVal))
        { // the isNumber() function is implemented below
            attrVal.nVal = atof(strVal);
        }
        else
        {
            return E_ATTRTYPEMISMATCH;
        }
    }
    else if (type == STRING)
    {
        strcpy(attrVal.sVal, strVal);
    }

    /*** Selecting records from the source relation ***/

    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);
    int srcNattrs = relCatEntry.numAttrs;

    char attrNames[srcNattrs][ATTR_SIZE];
    int attrTypes[srcNattrs];

    for (int i = 0; i < srcNattrs; i++)
    {

        AttrCatEntry attEntry;
        AttrCacheTable::getAttrCatEntry(srcRelId, i, &attEntry);
        strcpy(attrNames[i], attEntry.attrName);
        attrTypes[i] = attEntry.attrType;
    }
    ret = Schema::createRel(targetRel, srcNattrs, attrNames, attrTypes);
    if (ret != SUCCESS)
        return ret;
    int targetRelId = OpenRelTable::openRel(targetRel);
    if (targetRelId < 0 || targetRelId >=MAX_OPEN)
    {
        Schema::deleteRel(targetRel);
        return ret;
    }
    RelCacheTable::resetSearchIndex(srcRelId);
    AttrCacheTable::resetSearchIndex(srcRelId,attr);

    Attribute record[srcNattrs];
    while (BlockAccess::search(srcRelId, record, attr, attrVal, op) == SUCCESS)
    {
        //printf("search was succesful\n");
        ret = BlockAccess::insert(targetRelId, record);
        if (ret != SUCCESS)
        {
            //  printf("insert isnt happening\n");
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return ret;
        }
    }
    printf("%d\n",BPlusTree::getCompCount());
    Schema::closeRel(targetRel);
    return SUCCESS;
}
int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], int targetNoAttrs, char targetAttrs[][ATTR_SIZE])
{

    int srcRelId = OpenRelTable::getRelId(srcRel);
    if (srcRelId == E_RELNOTOPEN)
        return srcRelId;

    RelCatEntry srcRelCatBuf;
    RelCacheTable::getRelCatEntry(srcRelId, &srcRelCatBuf);
    int srcNoAttrs = srcRelCatBuf.numAttrs;

    int attrOffset[targetNoAttrs];
    int attrTypes[targetNoAttrs];
    int ret;
    for (int i = 0; i < targetNoAttrs; i++)
    {
        AttrCatEntry attrCatEntryBuf;
        ret = AttrCacheTable::getAttrCatEntry(srcRelId, targetAttrs[i], &attrCatEntryBuf);
        if (ret != SUCCESS)
            return ret;
        attrOffset[i] = attrCatEntryBuf.offset;
        attrTypes[i] = attrCatEntryBuf.attrType;
    }

    ret = Schema::createRel(targetRel, targetNoAttrs, targetAttrs, attrTypes);
    if (ret != SUCCESS)
    {
        return ret;
    }
    int targetRelId = OpenRelTable::openRel(targetRel);
    if (targetRelId < 0 || targetRelId > MAX_OPEN)
    {
        Schema::deleteRel(targetRel);
        return targetRelId;
    }
    Attribute record[srcNoAttrs];
    RelCacheTable::resetSearchIndex(srcRelId);
    while (BlockAccess::project(srcRelId, record) == SUCCESS)
    {
        Attribute projRecord[targetNoAttrs];
        for (int i = 0; i < targetNoAttrs; i++)
        {
            projRecord[i] = record[attrOffset[i]];
        }
        ret = BlockAccess::insert(targetRelId, projRecord);
        if (ret != SUCCESS)
        {
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return ret;
        }
    }
    Schema::closeRel(targetRel);
    return SUCCESS;
}
int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE])
{
    int srcRelId = OpenRelTable::getRelId(srcRel);
    if (srcRelId == E_RELNOTOPEN)
    {
        return srcRelId;
    }
    RelCatEntry srcRelCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId, &srcRelCatEntry);
    int srcNoAttrs = srcRelCatEntry.numAttrs;

    char attrNames[srcNoAttrs][ATTR_SIZE];
    int attrTypes[srcNoAttrs];
    for (int i = 0; i < srcNoAttrs; i++)
    {
        AttrCatEntry srcAttrCatEntry;
        AttrCacheTable::getAttrCatEntry(srcRelId, i, &srcAttrCatEntry);
        strcpy(attrNames[i], srcAttrCatEntry.attrName);
        attrTypes[i] = srcAttrCatEntry.attrType;
    }
    int ret = Schema::createRel(targetRel, srcNoAttrs, attrNames, attrTypes);
    if (ret != SUCCESS)
    {
        return ret;
    }
    int targetRelId = OpenRelTable::openRel(targetRel);
    if (targetRelId < 0)
    {
        Schema::deleteRel(targetRel);
        return targetRelId;
    }
    Attribute record[srcNoAttrs];
    RelCacheTable::resetSearchIndex(srcRelId);
    while (BlockAccess::project(srcRelId, record) == SUCCESS)
    {
        ret = BlockAccess::insert(targetRelId, record);
        if (ret != SUCCESS)
        {
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return ret;
        }
    }
    Schema::closeRel(targetRel);
    return SUCCESS;
}
// will return if a string can be parsed as a floating point number
bool isNumber(char *str)
{
    int len;
    float ignore;
    /*
      sscanf returns the number of elements read, so if there is no float matching
      the first %f, ret will be 0, else it'll be 1

      %n gets the number of characters read. this scanf sequence will read the
      first float ignoring all the whitespace before and after. and the number of
      characters read that far will be stored in len. if len == strlen(str), then
      the string only contains a float with/without whitespace. else, there's other
      characters.
    */

    int ret = sscanf(str, "%f %n", &ignore, &len);
    return ret == 1 && len == strlen(str);
}
int Algebra::insert(char relName[ATTR_SIZE], int nAttrs, char record[][ATTR_SIZE])
{
    // if relName is equal to "RELATIONCAT" or "ATTRIBUTECAT"
    // return E_NOTPERMITTED;
    if (strcmp(relName, RELCAT_RELNAME) == 0 ||
        strcmp(relName, ATTRCAT_RELNAME) == 0)
    {
        return E_NOTPERMITTED;
    }

    // get the relation's rel-id using OpenRelTable::getRelId() method
    int relId = OpenRelTable::getRelId(relName);

    // if relation is not open in open relation table, return E_RELNOTOPEN
    // (check if the value returned from getRelId function call = E_RELNOTOPEN)
    if (relId == E_RELNOTOPEN)
        return E_RELNOTOPEN;
    // get the relation catalog entry from relation cache

    // (use RelCacheTable::getRelCatEntry() of Cache Layer)
    RelCatEntry relCatBuff;
    RelCacheTable::getRelCatEntry(relId, &relCatBuff);
    /* if relCatEntry.numAttrs != numberOfAttributes in relation,
       return E_NATTRMISMATCH */
    if (relCatBuff.numAttrs != nAttrs)
    {
        return E_NATTRMISMATCH;
    }

    // let recordValues[numberOfAttributes] be an array of type union Attribute
    Attribute recordValues[nAttrs];
    /*
        Converting 2D char array of record values to Attribute array recordValues
     */
    // iterate through 0 to nAttrs-1: (let i be the iterator)
    int i = 0;
    for (; i < nAttrs; i++)
    {
        // get the attr-cat entry for the i'th attribute from the attr-cache
        // (use AttrCacheTable::getAttrCatEntry())
        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(relId, i, &attrCatEntry);
        // let type = attrCatEntry.attrType;
        int type = attrCatEntry.attrType;
        if (type == NUMBER)
        {
            // if the char array record[i] can be converted to a number
            // (check this using isNumber() function)
            if (isNumber(record[i]))
            {
                /* convert the char array to numeral and store it
                   at recordValues[i].nVal using atof() */
                recordValues[i].nVal = atof(record[i]);
            }
            else
            {
                return E_ATTRTYPEMISMATCH;
            }
        }
        else if (type == STRING)
        {
            strcpy(recordValues[i].sVal, record[i]);
            // copy record[i] to recordValues[i].sVal
        }
    }
    int retVal = BlockAccess::insert(relId, recordValues);
    // insert the record by calling BlockAccess::insert() function
    // let retVal denote the return value of insert call

    return retVal;
}
