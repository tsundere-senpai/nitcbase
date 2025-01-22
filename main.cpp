#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
// #include "FrontendInterface/FrontendInterface.h"
#include <iostream>
#include <cstring>
using namespace std;

void updateattrName(const char *RelName, const char *OldAttName, const char *NewAttName)
{

  RecBuffer attrCatBuffer(ATTRCAT_BLOCK);

  HeadInfo attrCatHeader;
  attrCatBuffer.getHeader(&attrCatHeader);
  // i=index of the record
  for (int i = 0; i < attrCatHeader.numEntries; i++)
  {
    Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
    attrCatBuffer.getRecord(attrCatRecord, i);

    if (strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, RelName) == 0 && strcmp(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, OldAttName) == 0)
    {
      strcpy(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, NewAttName);
      attrCatBuffer.setRecord(attrCatRecord, i);
      std::cout << "Update Complete!\n\n";
      break;
    }
    if (i == attrCatHeader.numSlots - 1)
    {
      i = -1;
      attrCatBuffer = RecBuffer(attrCatHeader.rblock);
      attrCatBuffer.getHeader(&attrCatHeader);
    }
  }
}
int main(int argc, char *argv[])
{
  Disk disk_run;
  StaticBuffer buffer;
  OpenRelTable cache;

  for (int relId = 0; relId < 2; relId++)
  {
    RelCatEntry relCatBuffer;
    RelCacheTable::getRelCatEntry(relId, &relCatBuffer);

    cout << "Relation Name: " << relCatBuffer.relName << endl;
    for (int attrIndex = 0; attrIndex < relCatBuffer.numAttrs; attrIndex++)
    {
      AttrCatEntry attrCatBuffer;
      AttrCacheTable::getAttrCatEntry(relId,attrIndex,&attrCatBuffer);
      const char *attrType = attrCatBuffer.attrType == NUMBER ? "NUMBER" : "STRING";
      printf("  %s:%s\n",attrCatBuffer.attrName,attrType);
    }
  }
}