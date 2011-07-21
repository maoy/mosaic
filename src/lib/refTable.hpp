/* $Id: refTable.hpp 211 2009-04-14 22:16:01Z maoy $ */

#ifndef __REFTABLE_HPP__
#define __REFTABLE_HPP__

#include <map>
#include <list>
#include <functional>
#include <algorithm>
#include <gc/gc_allocator.h>

#include <boost/assert.hpp>
#include "boost/tuple/tuple_comparison.hpp"
#include <boost/foreach.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include "tuple.hpp"
#include "table.hpp"

using boost::multi_index_container;
using namespace boost::multi_index;


//DBIndex forward definition
template<class E,
	 class PKey,
	 class SKey1=NoKey,
	 class SKey2=NoKey,
	 class SKey3=NoKey,
         class SKey4=NoKey
         >
struct DBIndex;

//DBIndex 1 key case
template<class E, class PKey>
struct DBIndex<E,PKey,NoKey,NoKey,NoKey,NoKey>{
  typedef EntryProjection<E,PKey> pkey_proj_type;
  //typedef EntryProjection<E,TimeStampKey> skey1_proj_type;
  typedef indexed_by<
    //hashed_unique<pkey_proj_type,TupleHash<typename pkey_proj_type::key_type>, ptrEqual<typename pkey_proj_type::key_type> >
    ordered_unique<tag<PKey>, pkey_proj_type,ptrLess<typename pkey_proj_type::key_type> >
    //,ordered_non_unique<tag<TimeStampKey>, skey1_proj_type,ptrLess<typename skey1_proj_type::key_type> >
    //,ordered_non_unique<tag<NoKey>,TimeProj<E> >
    > result_type;
};

//DBIndex 2 key case
template<class E, class PKey, class SKey1>
struct DBIndex<E,PKey,SKey1,NoKey,NoKey,NoKey>{
  typedef EntryProjection<E,PKey> pkey_proj_type;
  typedef EntryProjection<E,SKey1> skey1_proj_type;
  typedef indexed_by<
    ordered_unique<tag<PKey>, pkey_proj_type,ptrLess<typename pkey_proj_type::key_type> >,
    ordered_non_unique<tag<SKey1>, skey1_proj_type,ptrLess<typename skey1_proj_type::key_type> >
    > result_type;
};

//DBIndex 3 key case
template<class E, class PKey, class SKey1, class SKey2>
struct DBIndex<E,PKey,SKey1,SKey2,NoKey,NoKey>{
  typedef EntryProjection<E,PKey> pkey_proj_type;
  typedef EntryProjection<E,SKey1> skey1_proj_type;
  typedef EntryProjection<E,SKey2> skey2_proj_type;
  typedef indexed_by<
    ordered_unique<tag<PKey>, pkey_proj_type,ptrLess<typename pkey_proj_type::key_type> >,
    ordered_non_unique<tag<SKey1>, skey1_proj_type,ptrLess<typename skey1_proj_type::key_type> >,
    ordered_non_unique<tag<SKey2>, skey2_proj_type,ptrLess<typename skey2_proj_type::key_type> >
    > result_type;
};

//DBIndex 4 key case
template<class E, class PKey, class SKey1, class SKey2, class SKey3>
struct DBIndex<E,PKey,SKey1,SKey2,SKey3,NoKey>{
  typedef EntryProjection<E,PKey> pkey_proj_type;
  typedef EntryProjection<E,SKey1> skey1_proj_type;
  typedef EntryProjection<E,SKey2> skey2_proj_type;
  typedef EntryProjection<E,SKey3> skey3_proj_type;
  typedef indexed_by<
    ordered_unique<tag<PKey>, pkey_proj_type,ptrLess<typename pkey_proj_type::key_type> >,
    ordered_non_unique<tag<SKey1>, skey1_proj_type,ptrLess<typename skey1_proj_type::key_type> >,
    ordered_non_unique<tag<SKey2>, skey2_proj_type,ptrLess<typename skey2_proj_type::key_type> >,
    ordered_non_unique<tag<SKey3>, skey3_proj_type,ptrLess<typename skey3_proj_type::key_type> >
    > result_type;
};

//DBIndex 5 key case
template<class E, class PKey, class SKey1, class SKey2, class SKey3, class SKey4>
struct DBIndex{
  typedef EntryProjection<E,PKey> pkey_proj_type;
  typedef EntryProjection<E,SKey1> skey1_proj_type;
  typedef EntryProjection<E,SKey2> skey2_proj_type;
  typedef EntryProjection<E,SKey3> skey3_proj_type;
  typedef EntryProjection<E,SKey4> skey4_proj_type;
  typedef indexed_by<
    ordered_unique<tag<PKey>, pkey_proj_type,ptrLess<typename pkey_proj_type::key_type> >,
    ordered_non_unique<tag<SKey1>, skey1_proj_type,ptrLess<typename skey1_proj_type::key_type> >,
    ordered_non_unique<tag<SKey2>, skey2_proj_type,ptrLess<typename skey2_proj_type::key_type> >,
    ordered_non_unique<tag<SKey3>, skey3_proj_type,ptrLess<typename skey3_proj_type::key_type> >,
    ordered_non_unique<tag<SKey4>, skey4_proj_type,ptrLess<typename skey4_proj_type::key_type> >
    > result_type;
};

//////////////////////////////////////////
#define REFTABLE_TEMPLATE template<class TupleT, class PKey, class SKey1,class SKey2,class SKey3,class SKey4>
#define REFTABLE_TEMPLATE_PREFIX RefTable<TupleT,PKey,SKey1,SKey2,SKey3,SKey4>

template<class TupleT, class PKey, class SKey1=NoKey, class SKey2=NoKey, class SKey3=NoKey, class SKey4=NoKey> 
class RefTable: public BaseTable{
  /////// types
public:
  typedef TupleT tuple_type;
  typedef TupleProjection<TupleT,PKey> pkey_proj_type;
  typedef typename pkey_proj_type::key_type key_type;
protected:
  struct Entry{
    typedef TupleT tuple_type;
    TupleT* tuple;
    //time, ref count, others
    int refCount;
    //ptime exp_time;
    explicit Entry(TupleT* t=NULL):tuple(t),refCount(1)
    {}
    ~Entry(){};
  };
  virtual bool insertTuple(Tuple* t);
  virtual bool eraseTuple(Tuple* t, bool match_key_only);
  virtual BaseTablePtr clone();

public:
  typedef Entry entry_type;
  typedef multi_index_container<
    Entry,
    typename DBIndex<Entry,PKey,SKey1,SKey2,SKey3,SKey4>::result_type,
    gc_allocator<Entry>
    > container_type;
  typedef typename container_type::template nth_index<0>::type pindex_type;
  ///////// end of types
  
  //////// data
protected:
  container_type _data;
  bool _useRefCount;
  //////// end of data

  //////// methods
public:
  //ctors
  explicit RefTable(EventQueue* q = NULL, bool useRefCount=true)
    :BaseTable(q), _useRefCount(useRefCount)
  {}
  //copy constructor: allow duplicate a table
  explicit RefTable(const RefTable
                    //<TupleT,PKey, SKey1, SKey2,SKey3, SKey4>
                    & r):
    BaseTable(r._queue)
    , _data(r._data) //copy each entry over
    , _useRefCount(r._useRefCount)
  {}

  TupleT* primaryLookup(const key_type* key) const;
  //get begin and end for a secondary key
  template <class Key>
  std::pair<
    typename container_type::template index<Key>::type::const_iterator,
    typename container_type::template index<Key>::type::const_iterator>
  forEach() const{
    return std::make_pair(_data.get<Key>().begin(), _data.get<Key>().end());
  }
  //get begin and end for a primary key
  std::pair<
    typename container_type::const_iterator,
    typename container_type::const_iterator>
  forEach() const{
    return std::make_pair(_data.begin(), _data.end());
  }
  std::size_t size() {return _data.size();}
  std::size_t size() const {return _data.size();}
  template <class SKey, class key_type>
  std::pair<
    typename container_type::template index<SKey>::type::const_iterator,
    typename container_type::template index<SKey>::type::const_iterator>
  secondaryLookup(SKey& _, key_type* key) const{
    return _data.get<SKey>().equal_range(key);
  }

  void dump();
};

REFTABLE_TEMPLATE
void REFTABLE_TEMPLATE_PREFIX::dump(){
  std::cout << "dumping refTable.." << std::endl;
  for (typename pindex_type::iterator it=_data.get<0>().begin();it!=_data.get<0>().end();++it){
    std::cout << "  " << *(*it).tuple
              << "\t\t[RefCnt=" 
              << (*it).refCount 
              << (_useRefCount? "":"(unused)")
              << ']' 
              << std::endl;
  }
}

REFTABLE_TEMPLATE
bool REFTABLE_TEMPLATE_PREFIX::insertTuple(Tuple* tp){
  TupleT* t = tuple_ptr_cast<TupleT*>(tp);
  //Entry* e = new Entry(t);
  Entry e(t);
  const key_type* key = NULL;
  key = pkey_proj_type()(t);
  std::pair<typename pindex_type::iterator, bool> r
    =_data.get<0>().insert(e);
  if (!r.second) {// existed
    if ((*(*r.first).tuple) ==*t) {
      //duplicate tuple
      //std::cout << "found duplicate tuple" << *t << std::endl;
      e.refCount = (*r.first).refCount+1;
      _data.get<0>().replace(r.first, e);
      return false;
    }else {
      //same key but different value. delete the old and add the new one
      if (this->_queue) {
        Event ev(Event::DELETED, (*r.first).tuple);
        this->_queue->enqueue(ev);
      }
      _data.get<0>().replace(r.first, e);
    }
  }
  if (this->_queue){
    Event ev(Event::INSERTED, t);
    this->_queue->enqueue(ev);
  }
  return true;  
}

REFTABLE_TEMPLATE
bool REFTABLE_TEMPLATE_PREFIX::eraseTuple(Tuple* tp, bool match_key_only){
  TupleT* t = tuple_ptr_cast<TupleT*>(tp);
  const key_type* key = pkey_proj_type()(t);
  typename pindex_type::iterator it;
  it = _data.get<0>().find(key);
  if (it == _data.get<0>().end()) //do not exist
    return false;
  if ( !match_key_only && (*((*it).tuple) != *t) ) // not the same tuple
    return false;
  t = it->tuple;
  int refCount = (*it).refCount;
  BOOST_ASSERT( refCount > 0);
  --refCount;
  if (!_useRefCount || (refCount==0) ) {
    _data.get<0>().erase(it);
    if (this->_queue){
      Event ev(Event::DELETED,  t);
      this->_queue->enqueue(ev);
    }
    return true;
  }
  else{
    Entry e = (*it);
    e.refCount = refCount;
    _data.get<0>().replace(it, e);
  }
  return false;
}

REFTABLE_TEMPLATE
BaseTablePtr REFTABLE_TEMPLATE_PREFIX::clone()
{
  return new REFTABLE_TEMPLATE_PREFIX(*this);
}

REFTABLE_TEMPLATE
TupleT* REFTABLE_TEMPLATE_PREFIX::primaryLookup(const key_type* key) const{
  typename pindex_type::const_iterator it;
  it =  _data.get<0>().find(key);
  if (it == _data.get<0>().end())
    return NULL;
  return (*it).tuple;
}
///////////////////////////////////////////////////////////////////


#endif
