/* $Id: table.cpp 210 2009-04-14 22:14:22Z maoy $ */

#include "table.hpp"

bool BaseTable::commit(){
  BOOST_FOREACH( tuple_type* t,_insertionList )
  {
    insertTuple(t);
  }
  _insertionList.clear();
  BOOST_FOREACH( tuple_type* t,_removalList )
  {
    eraseTuple(t, false);
  }
  _removalList.clear();
  BOOST_FOREACH( tuple_type* t,_removalKeyList )
  {
    eraseTuple(t, true);
  }
  _removalKeyList.clear();
  return true;
}
