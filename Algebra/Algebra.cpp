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
   // printf("%d\n",BPlusTree::getCompCount());
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
int Algebra::join(char srcRelation1[ATTR_SIZE], char srcRelation2[ATTR_SIZE], char targetRelation[ATTR_SIZE], char attribute1[ATTR_SIZE], char attribute2[ATTR_SIZE]) {

    // get the srcRelation1's rel-id using OpenRelTable::getRelId() method
    int relIdSrc1=OpenRelTable::getRelId(srcRelation1);
    // get the srcRelation2's rel-id using OpenRelTable::getRelId() method
    int relIdSrc2=OpenRelTable::getRelId(srcRelation2);
   // printf("relid 1%d relid2 %d\n",relIdSrc1,relIdSrc2);
    // if either of the two source relations is not open
    //     return E_RELNOTOPEN
    if(relIdSrc1==E_RELNOTOPEN||relIdSrc2==E_RELNOTOPEN)
        return E_RELNOTOPEN;

    AttrCatEntry attrCatEntry1, attrCatEntry2;
    // get the attribute catalog entries for the following from the attribute cache
    // (using AttrCacheTable::getAttrCatEntry())
    // - attrCatEntry1 = attribute1 of srcRelation1
    // - attrCatEntry2 = attribute2 of srcRelation2
    int ret1= AttrCacheTable::getAttrCatEntry(relIdSrc1,attribute1,&attrCatEntry1);
    int ret2= AttrCacheTable::getAttrCatEntry(relIdSrc2,attribute2,&attrCatEntry2);

    // if attribute1 is not present in srcRelation1 or attribute2 is not
    // present in srcRelation2 (getAttrCatEntry() returned E_ATTRNOTEXIST)
    //     return E_ATTRNOTEXIST.
    // printf("%s\n",attrCatEntry1.attrName);
    // printf("%s\n",attrCatEntry2.attrName);
    if(ret1==E_ATTRNOTEXIST||ret2==E_ATTRNOTEXIST)
        return E_ATTRNOTEXIST;
     //   printf("SUCCESS open attr\n");
    // if attribute1 and attribute2 are of different types return E_ATTRTYPEMISMATCH
    if(attrCatEntry1.attrType!=attrCatEntry2.attrType)
        return E_ATTRTYPEMISMATCH;
    // iterate through all the attributes in both the source relations and check if
    // there are any other pair of attributes other than join attributes
    // (i.e. attribute1 and attribute2) with duplicate names in srcRelation1 and
    // srcRelation2 (use AttrCacheTable::getAttrCatEntry())
    // If yes, return E_DUPLICATEATTR
   RelCatEntry relCatEntry1,relCatEntry2;
   RelCacheTable::getRelCatEntry(relIdSrc1,&relCatEntry1);
   RelCacheTable::getRelCatEntry(relIdSrc2,&relCatEntry2);


    

    // get the relation catalog entries for the relations from the relation cache
    // (use RelCacheTable::getRelCatEntry() function)

    int numOfAttributes1 = relCatEntry1.numAttrs;
    int numOfAttributes2 = relCatEntry2.numAttrs;
    for (int attrIdx  = 0; attrIdx < numOfAttributes1; attrIdx++)
    {
        AttrCatEntry tempEntry1;
        AttrCacheTable::getAttrCatEntry(relIdSrc1,attrIdx,&tempEntry1);
        if(strcmp(tempEntry1.attrName,attribute1)==0)
            continue;
        for(int attrIdx2=0;attrIdx2<numOfAttributes2;attrIdx2++){
            AttrCatEntry tempEntry2;
            AttrCacheTable::getAttrCatEntry(relIdSrc2,attrIdx2,&tempEntry2);
            if(strcmp(tempEntry2.attrName,attribute2)==0)
                continue;
            if(strcmp(tempEntry1.attrName,tempEntry2.attrName)==0)
                return E_DUPLICATEATTR;
            
        }
    }
 //   printf("validation complete\n");
    // if rel2 does not have an index on attr2
    //     create it using BPlusTree:bPlusCreate()
    //     if call fails, return the appropriate error code
    //     (if your implementation is correct, the only error code that will
    //      be returned here is E_DISKFULL)
    int rootBlock=attrCatEntry2.rootBlock;
    if(rootBlock==-1){
       ret1= BPlusTree::bPlusCreate(relIdSrc2,attribute2);
       if(ret1==E_DISKFULL) return E_DISKFULL;

       rootBlock=attrCatEntry2.rootBlock;

    }
 //   printf("rootblock done\n");
    int numOfAttributesInTarget = numOfAttributes1 + numOfAttributes2 - 1;
    // Note: The target relation has number of attributes one less than
    // nAttrs1+nAttrs2 (Why?)

    // declare the following arrays to store the details of the target relation
    char targetRelAttrNames[numOfAttributesInTarget][ATTR_SIZE];
    int targetRelAttrTypes[numOfAttributesInTarget];

    // iterate through all the attributes in both the source relations and
    // update targetRelAttrNames[],targetRelAttrTypes[] arrays excluding attribute2
    // in srcRelation2 (use AttrCacheTable::getAttrCatEntry())
    for(int attrInd=0;attrInd<numOfAttributes1;attrInd++){
        AttrCatEntry attrEntry1;
        AttrCacheTable::getAttrCatEntry(relIdSrc1,attrInd,&attrEntry1);
        strcpy(targetRelAttrNames[attrInd],attrEntry1.attrName);
        targetRelAttrTypes[attrInd]=attrEntry1.attrType;
    }
  //  printf("Copying data attr1 done done\n");
    for (int attrindex = 0, flag = 0; attrindex < numOfAttributes2; attrindex++)
    {
        AttrCatEntry attrcatentry; 
        AttrCacheTable::getAttrCatEntry(relIdSrc2, attrindex ,&attrcatentry);

        if (strcmp(attribute2, attrcatentry.attrName) == 0)
        {
            flag = 1;
            continue;
        }

        strcpy(targetRelAttrNames[numOfAttributes1 + attrindex-flag], attrcatentry.attrName);
        targetRelAttrTypes[numOfAttributes1 + attrindex-flag] = attrcatentry.attrType;
    }    
    // for (int i = 0; i <( numOfAttributes1+numOfAttributes2)-1; i++)
    // {
    //     printf("%s\n",targetRelAttrNames[i]);
    // }
    
  //  printf("Copying data attr2 done\n");
    // create the target relation using the Schema::createRel() function
    
    
   ret1= Schema::createRel(targetRelation,numOfAttributesInTarget,targetRelAttrNames,targetRelAttrTypes);
    // if createRel() returns an error, return that error
    if(ret1!=SUCCESS)
        return ret1;
    // Open the targetRelation using OpenRelTable::openRel()
    int relId=OpenRelTable::openRel(targetRelation);
    // RelCatEntry tar;
    // RelCacheTable::getRelCatEntry(relId,&tar);
    // AttrCatEntry tarAtt;
    // for (int i = 0; i < tar.numAttrs; i++)
    // {
    //     AttrCacheTable::getAttrCatEntry(relId,i,&tarAtt);
    //     printf("%s ",tarAtt.attrName);
    // }
    // printf("\n");  
    // if openRel() fails (No free entries left in the Open Relation Table)
    if(relId<0||relId>=MAX_OPEN)
    {
        // delete target relation by calling Schema::deleteRel()
        // return the error code
        Schema::deleteRel(targetRelation);
        return relId;
    }

    Attribute record1[numOfAttributes1];
    Attribute record2[numOfAttributes2];
    Attribute targetRecord[numOfAttributesInTarget];

    RelCacheTable::resetSearchIndex(relIdSrc1);
    // this loop is to get every record of the srcRelation1 one by one
    while (BlockAccess::project(relIdSrc1, record1) == SUCCESS) {
       // printf("Project done\n");
        // reset the search index of `srcRelation2` in the relation cache
        // using RelCacheTable::resetSearchIndex()
        RelCacheTable::resetSearchIndex(relIdSrc2);

        // reset the search index of `attribute2` in the attribute cache
        // using AttrCacheTable::resetSearchIndex()
        AttrCacheTable::resetSearchIndex(relIdSrc2,attribute2);

        // this loop is to get every record of the srcRelation2 which satisfies
        //the following condition:
        // record1.attribute1 = record2.attribute2 (i.e. Equi-Join condition)
        while (BlockAccess::search(relIdSrc2, record2, attribute2, record1[attrCatEntry1.offset], EQ ) == SUCCESS ) {
          //  printf("search done\n")
;            // copy srcRelation1's and srcRelation2's attribute values(except
            // for attribute2 in rel2) from record1 and record2 to targetRecord
            for (int attrindex = 0; attrindex < numOfAttributes1; attrindex++)
                targetRecord[attrindex] = record1[attrindex];
            for (int attrindex = 0, flag = 0; attrindex < numOfAttributes2; attrindex++)
                {
                    if (attrindex == attrCatEntry2.offset)
                    {
                        flag = 1;
                        continue;
                    }
                    targetRecord[attrindex + numOfAttributes1-flag] = record2[attrindex];
                }
            // insert the current record into the target relation by calling
            // BlockAccess::insert()
            ret1=BlockAccess::insert(relId,targetRecord);
           // printf("Insert done\n");
            // (insert should fail only due to DISK being FULL)
            if(ret1==E_DISKFULL) {

                // close the target relation by calling OpenRelTable::closeRel()
                // delete targetRelation (by calling Schema::deleteRel())
                Schema::closeRel(targetRelation);
                Schema::deleteRel(targetRelation);
                return E_DISKFULL;
            }
        }
    }
    OpenRelTable::closeRel(relId);
    // close the target relation by calling OpenRelTable::closeRel()
    return SUCCESS;
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
