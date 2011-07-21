/* $Id: workspace.hpp 211 2009-04-14 22:16:01Z maoy $ */

#ifndef __WORKSPACE_HPP__
#define __WORKSPACE_HPP__

#include <tr1/unordered_set>
#include <tr1/unordered_map>

#include <gc/gc_cpp.h>
#include <gc/gc_allocator.h>

#include "table.hpp"

typedef uint32_t TableID;

class Workspace;

typedef Workspace* WorkspacePtr;

class Workspace: public gc {
private:
  // data members
  std::tr1::unordered_map<TableID
                          ,BaseTablePtr
                          ,std::tr1::hash<TableID>
                          ,std::equal_to<TableID>
                          ,gc_allocator<std::pair<const TableID, BaseTablePtr> > 
                          > tables;

  std::tr1::unordered_set<TableID
                          , std::tr1::hash<TableID>
                          , std::equal_to<TableID>
                          , gc_allocator<TableID>
                          > dirty_tables;
  
public:
  //constructors & destructors
  Workspace(){}
  virtual ~Workspace(){}
  //branching out from another workspace: copy over the table map
  Workspace(const Workspace& w)
    : tables(w.tables)
    , dirty_tables()
  {}
  
  //methods
  WorkspacePtr clone();

  //to be used in constructing the initial tables
  void addTable( TableID, BaseTablePtr );
  
  //return the read pointer to the table
  BaseTablePtr getReadTable( TableID );
  
  //return the write pointer to the table
  //duplicate table if needed, and return the pointer;
  BaseTablePtr getWriteTable( TableID );
};


#endif
