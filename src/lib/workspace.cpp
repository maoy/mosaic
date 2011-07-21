/* $Id: workspace.cpp 210 2009-04-14 22:14:22Z maoy $ */

#include "workspace.hpp"

WorkspacePtr Workspace::clone()
{
  return new Workspace(*this);
}

//to be used in constructing the initial tables
void Workspace::addTable( TableID id, BaseTablePtr ptr )
{
  tables[id] = ptr;
}

  
  //return the read pointer to the table
BaseTablePtr Workspace::getReadTable( TableID id )
{
  return tables[id];
}
  
//return the write pointer to the table
//duplicate table if needed, and return the pointer
BaseTablePtr Workspace::getWriteTable( TableID id ){
  if ( dirty_tables.find(id) != dirty_tables.end() ) {
    //already duplicated
    return tables[id];
  }
  // not dirty yet, mark it dirty
  dirty_tables.insert(id);
  BaseTablePtr p = tables[id]->clone();
  tables[id] = p;
  return p;
}

