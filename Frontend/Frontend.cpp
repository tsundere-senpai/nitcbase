#include "Frontend.h"

#include <cstring>
#include <iostream>

int Frontend::create_table(char relname[ATTR_SIZE], int no_attrs, char attributes[][ATTR_SIZE],
						   int type_attrs[])
{
	// Schema::createRel
	return Schema::createRel(relname, no_attrs, attributes, type_attrs);
	// return SUCCESS;
}

int Frontend::drop_table(char relname[ATTR_SIZE])
{
	// Schema::deleteRel
	return Schema::deleteRel(relname);
	return SUCCESS;
}

int Frontend::open_table(char relname[ATTR_SIZE])
{
	// Schema::openRel
	return Schema::openRel(relname);
	// return SUCCESS;
}

int Frontend::close_table(char relname[ATTR_SIZE])
{
	// Schema::closeRel
	return Schema::closeRel(relname);
	// return SUCCESS;
}

int Frontend::alter_table_rename(char relname_from[ATTR_SIZE], char relname_to[ATTR_SIZE])
{
	// Schema::renameRel
	return Schema::renameRel(relname_from, relname_to);
}

int Frontend::alter_table_rename_column(char relname[ATTR_SIZE], char attrname_from[ATTR_SIZE],
										char attrname_to[ATTR_SIZE])
{
	// Schema::renameAttr
	return Schema::renameAttr(relname, attrname_from, attrname_to);
	// return SUCCESS;
}

int Frontend::create_index(char relname[ATTR_SIZE], char attrname[ATTR_SIZE])
{
	// Schema::createIndex

	return Schema::createIndex(relname, attrname);
}

int Frontend::drop_index(char relname[ATTR_SIZE], char attrname[ATTR_SIZE])
{
	// Schema::dropIndex
	return Schema::dropIndex(relname, attrname);
}

int Frontend::insert_into_table_values(char relname[ATTR_SIZE], int attr_count, char attr_values[][ATTR_SIZE])
{
	// Algebra::insert
	return Algebra::insert(relname, attr_count, attr_values);
	// return SUCCESS;
}

int Frontend::select_from_table(char relname_source[ATTR_SIZE], char relname_target[ATTR_SIZE])
{
	// Algebra::project
	return Algebra::project(relname_source, relname_target);
	return SUCCESS;
}

int Frontend::select_attrlist_from_table(char relname_source[ATTR_SIZE], char relname_target[ATTR_SIZE],
										 int attr_count, char attr_list[][ATTR_SIZE])
{
	// Algebra::project
	return Algebra::project(relname_source, relname_target, attr_count, attr_list);
	return SUCCESS;
}

// int Frontend::select_from_table_where(char relname_source[ATTR_SIZE], char relname_target[ATTR_SIZE],
//                                       char attribute[ATTR_SIZE], int op, char value[ATTR_SIZE]) {
//   // Algebra::select

//   return SUCCESS;
// }
int Frontend::select_from_table_where(char relname_source[ATTR_SIZE], char relname_target[ATTR_SIZE],
									  char attribute[ATTR_SIZE], int op, char value[ATTR_SIZE])
{
	return Algebra::select(relname_source, relname_target, attribute, op, value);
}
int Frontend::select_attrlist_from_table_where(char relname_source[ATTR_SIZE], char relname_target[ATTR_SIZE],
											   int attr_count, char attr_list[][ATTR_SIZE],
											   char attribute[ATTR_SIZE], int op, char value[ATTR_SIZE])
{
	// Algebra::select + Algebra::project??
	int ret = Algebra::select(relname_source, "temp", attribute, op, value);
	if (ret != SUCCESS)
		return ret;

	ret = OpenRelTable::openRel("temp");
	if (ret < 0 || ret >= MAX_OPEN)
	{
		Schema::closeRel("temp");
		Schema::deleteRel("temp");
		return ret;
	}
	ret = Algebra::project("temp", relname_target, attr_count, attr_list);
	if (ret != SUCCESS)
	{
		Schema::closeRel("temp");
		Schema::deleteRel("temp");
		return ret;
	}
	Schema::closeRel("temp");
	Schema::deleteRel("temp");

	return SUCCESS;

	return SUCCESS;
}

int Frontend::select_from_join_where(char relname_source_one[ATTR_SIZE], char relname_source_two[ATTR_SIZE],
									 char relname_target[ATTR_SIZE],
									 char join_attr_one[ATTR_SIZE], char join_attr_two[ATTR_SIZE])
{
	// Algebra::join
	return Algebra::join(relname_source_one, relname_source_two, relname_target, join_attr_one, join_attr_two);
	// return SUCCESS;
}

int Frontend::select_attrlist_from_join_where(char relname_source_one[ATTR_SIZE], char relname_source_two[ATTR_SIZE],
											  char relname_target[ATTR_SIZE], char join_attr_one[ATTR_SIZE],
											  char join_attr_two[ATTR_SIZE], int attr_count, char attr_list[][ATTR_SIZE])
{

	// Call join() method of the Algebra Layer with correct arguments to
	// create a temporary target relation with name TEMP.
	int ret = Algebra::join(relname_source_one, relname_source_two, "temp_rel",  join_attr_one,join_attr_two);
	AttrCatEntry attrcat;
	// TEMP results from the join of the two source relation (and hence it
	// contains all attributes of the source relations except the join attribute
	// of the second source relation)
	if (ret != SUCCESS)
	{
		
		return ret;
	}
	// Return Error values, if not successful
	
	// Open the TEMP relation using OpenRelTable::openRel()
	// if open fails, delete TEMP relation using Schema::deleteRel() and
	// return the error code
	ret = Schema::openRel("temp_rel");
	if (ret!=SUCCESS)
	{
		Schema::deleteRel("temp_rel");
		return ret;
	}
	// int rel=OpenRelTable::getRelId("temp_rel");
	// printf("%d relid\n",rel);
	// RelCatEntry rel1;
	// RelCacheTable::getRelCatEntry(rel,&rel1);
	// for (int i = 0; i < rel1.numAttrs; i++)
	// {
	// 	/* code */
	// 	AttrCacheTable::getAttrCatEntry(rel,i,&attrcat);
	// 	printf("%s ",attrcat.attrName);
	// }
	// printf("\n");

	// Call project() method of the Algebra Layer with correct arguments to
	// create the actual target relation from the TEMP relation.
	// (The final target relation contains only those attributes mentioned in attr_list)
	ret=Algebra::project("temp_rel",relname_target,attr_count,attr_list);
	Schema::closeRel("temp_rel");
	Schema::deleteRel("temp_rel");
	return ret;
	// close the TEMP relation using OpenRelTable::closeRel()
	// delete the TEMP relation using Schema::deleteRel()

	// Return Success or Error values appropriately
}
int Frontend::custom_function(int argc, char argv[][ATTR_SIZE])
{
	// argc gives the size of the argv array
	// argv stores every token delimited by space and comma

	// implement whatever you desire

	return SUCCESS;
}