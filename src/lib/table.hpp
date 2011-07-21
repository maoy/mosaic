/* $Id: table.hpp 225 2009-06-16 20:49:02Z maoy $ */

#ifndef __TABLE_HPP__
#define __TABLE_HPP__

#include <list>
//#include <functional>
#include <algorithm>
#include <gc/gc_allocator.h>

#include "tuple.hpp"
#include "event.hpp"
#include <boost/foreach.hpp>

///////////////////////////////
struct NoKey{};

template<class E, class Key> class EntryProjection;

template<class E, class Key>
class EntryProjection{
private:
  typedef TupleProjection<typename E::tuple_type,Key> proj_type;
  proj_type p;
public:
  typedef typename proj_type::result_type result_type;
  typedef typename proj_type::key_type key_type;
  result_type operator()(const E& e) const{
    return p(e.tuple);
  }
};

////////////////
template<class E, int Pos>
struct FieldExtraction{
  typedef typename boost::tuples::element<Pos,typename E::tuple_type>::type result_type;
  typedef E entry_type;
  typedef typename entry_type::tuple_type tuple_type;
  result_type operator()(const E& e)const{
    return boost::get<Pos>(*(e.tuple));
  }
};

/* the following doesn't compile..
template <class T>
struct test
{
  typedef T type;
};
#include <boost/mpl/transform.hpp>
using namespace boost::mpl;
template<class E, class PKey, class SKeys>
struct Indices{
  typedef boost::mpl::transform< 
    SKeys,
    ordered_non_unique<
      //      test<mpl_::_1>,
      //tag<mpl_::_1>, 
      typename EntryProjection<E,mpl_::_1>::key_type,
      ptrLess<typename EntryProjection<E,mpl_::_1>::key_type>
      >
    >::type result_type;
};
*/
///////////////////////////////
class BaseTable;
typedef BaseTable* BaseTablePtr;

class BaseTable:public ICommitable, public gc {
public:
  typedef Tuple tuple_type;
  typedef std::list<tuple_type*, gc_allocator<tuple_type*> > action_list_type;

  virtual bool insertTuple(tuple_type*)=0;
  virtual bool eraseTuple(tuple_type*, bool match_key_only)=0;
  virtual std::size_t size()=0;
  virtual BaseTablePtr clone()=0;
private:
  action_list_type _insertionList;
  action_list_type _removalList;
  action_list_type _removalKeyList;

public:
  BaseTable(EventQueue* q): _queue(q){}

  virtual ~BaseTable(){}
  void insert(tuple_type* t, EventQueue* q = NULL){
    if (!q) 
      q = _queue;
    else
      _queue = q; //FIXME
    if (q)
      q->touchTable(this);
    _insertionList.push_back(t);
  }
  void erase(tuple_type* t, EventQueue* q = NULL) {
    if (!q) 
      q = _queue;
    else
      _queue = q; //FIXME
    if (q)
      q->touchTable(this);
    _removalList.push_back(t);
  }
  /**
   * erase based on primary key i.e. ignore non-key positions in the tuple
   */
  void eraseByKey(tuple_type* t){
    if (_queue)
      _queue->touchTable(this);
    _removalKeyList.push_back(t);
  }
  bool commit();
protected:
  EventQueue* _queue;

};


///////////////////////////////////////////////////////////////////

/*
  for each t_level in tablename[it_begin_level .. it_end_level]
  this is intended to be used in TABLE_FOREACH, 
  TABLE_SECONDARY_LOOKUP, and TABLE_SECONDARY_NOTIN only
*/
#define TABLE_ITER(tablename,level) \
  Tuple_##tablename* t##level;						\
  for (iter_type_##level it##level = it_begin_##level;			\
       (!(it##level == it_end_##level))?(t##level=(*it##level).tuple,true):false; \
       ++it##level)

#define TABLE_FOREACH(tablename,level) \
  typedef Table_##tablename::container_type::const_iterator iter_type_##level; \
  iter_type_##level it_begin_##level, it_end_##level;                   \
  tie(it_begin_##level,it_end_##level) = t_##tablename.forEach();       \
  TABLE_ITER(tablename,level)


#define TABLE_SECONDARY_LOOKUP(tablename,level,key,key_type) \
  typedef Table_##tablename::container_type::index< key_type >::type::const_iterator iter_type_##level; \
  iter_type_##level it_begin_##level, it_end_##level;			\
  key_type _key_inst_##level;						\
  tie(it_begin_##level,it_end_##level) = t_##tablename.secondaryLookup(_key_inst_##level, key); \
  TABLE_ITER(tablename,level)

//for each unique secondary key: used in aggregation
#define TABLE_FOREACH_KEY(tablename,level,key_type) \
  typedef Table_##tablename::container_type::index< key_type >::type::const_iterator iter_type_##level; \
  iter_type_##level it_begin_##level, it_end_##level, it_next_##level; \
  typedef EntryProjection<iter_type_##level::value_type,key_type> proj_type_##level; \
  proj_type_##level _proj_##level;                                      \
  typedef proj_type_##level::result_type key_inst_type_##level;         \
  key_inst_type_##level  _key_##level;                                  \
  key_type _dummy_key_##level;                                          \
  tie(it_begin_##level,it_end_##level) = t_##tablename.forEach< key_type >(); \
  for (iter_type_##level it##level = it_begin_##level;                  \
       (!(it##level==it_end_##level))?                                  \
         (_key_##level = _proj_##level(*it##level),                     \
          tie(it_begin_##level,it_next_##level) =                       \
          t_##tablename.secondaryLookup(_dummy_key_##level, _key_##level), \
          true):false;                                                  \
       it##level=it_next_##level)

#define TABLE_PRIMARY_LOOKUP(tablename,level,key) \
  Tuple_##tablename* t##level =			  \
    t_##tablename.primaryLookup(key);		  \
  if (t##level)
  
#define TABLE_PRIMARY_NOTIN(tablename,level,key)  \
  Tuple_##tablename* t##level =			  \
    t_##tablename.primaryLookup(key);		  \
  if (!t##level)

#define TABLE_SECONDARY_NOTIN(tablename,level,key,key_type) \
  typedef Table_##tablename::container_type::index< key_type >::type::const_iterator iter_type_##level; \
  iter_type_##level it_begin_##level, it_end_##level;			\
  key_type _key_inst_##level;						\
  tie(it_begin_##level,it_end_##level) = t_##tablename.secondaryLookup(_key_inst_##level, key); \
  if ( it_begin_##level == it_end_##level )

#endif
