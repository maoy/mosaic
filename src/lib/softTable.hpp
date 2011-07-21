/* $Id: softTable.hpp 211 2009-04-14 22:16:01Z maoy $ */

#ifndef __SOFTTABLE_HPP__
#define __SOFTTABLE_HPP__

#include <list>
#include <gc/gc_allocator.h>

#include <boost/foreach.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/ordered_index.hpp>
//#include <boost/multi_index/hashed_index.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "tuple.hpp"
#include "runtime.hpp"
#include "time.hpp"
#include "table.hpp"

using boost::multi_index_container;
using namespace boost::multi_index;
using boost::posix_time::ptime;
using boost::posix_time::time_duration;

/*When casting an entry to the TimeStampKey,
  the pointer of the expire time is returned for
  the index to eliminate expired tuples
 */
struct TimeStampKey{};
template <class E>
struct EntryProjection<E,TimeStampKey>{
  typedef ptime key_type;
  typedef const ptime* result_type;
  const ptime* operator()(const E& e) const{
    return &(e.exp_time);
  }
};

//////////////////////////////////////////
#define SOFTTABLE_TEMPLATE template<class TupleT, class PKey, class SKey1, class SKey2, class SKey3>
#define SOFTTABLE_TEMPLATE_PREFIX SoftTable<TupleT,PKey,SKey1,SKey2,SKey3>

template<class TupleT, class PKey, class SKey1=NoKey,class SKey2=NoKey,class SKey3=NoKey>
class SoftTable: public BaseTable{
  //////////typedefs
public:
  typedef TupleT tuple_type;
  typedef TupleProjection<TupleT,PKey> pkey_proj_type;
  typedef typename pkey_proj_type::key_type key_type;
  const static int minimum_flush_period = 1;
protected:
  struct Entry{
    typedef TupleT tuple_type;
    TupleT* tuple;
    //time, ref count, other metadata
    ptime exp_time;
    explicit Entry(TupleT* t=NULL):tuple(t)
    {}
    ~Entry(){};
  };
public:
  typedef Entry entry_type;
  typedef multi_index_container<
    Entry,
    typename DBIndex<Entry,PKey,TimeStampKey,SKey1,SKey2,SKey3>::result_type,
    gc_allocator<Entry>
    > container_type;
  typedef typename container_type::template nth_index<0>::type pindex_type;
  typedef typename container_type::template index<TimeStampKey>::type exp_time_idx_type;

  ///////////data
protected:
  container_type _data;
  time_duration _timeout;
  std::size_t _maxSize;
  deadline_timer _flush_timer;
  ///////////methods
public: 
  explicit SoftTable(TimeDuration timeout, std::size_t maxSize=0, EventQueue* q = NULL)
    :BaseTable(q),_timeout(timeout.get()), _maxSize(maxSize)
    ,_flush_timer(iosv)
  { flush_timercb();}

  //copy constructor: allow duplicate a table
  explicit SoftTable(const SoftTable& t)
    : BaseTable(t._queue)
    , _data(t._data) //copy each entry over
    , _timeout(t._timeout)
    , _maxSize(t._maxSize)
    , _flush_timer(iosv)
  {}
  

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
  TupleT* primaryLookup(const key_type* key);
  template <class SKey, class key_type>
  std::pair<
    typename container_type::template index<SKey>::type::const_iterator,
    typename container_type::template index<SKey>::type::const_iterator>
  secondaryLookup(SKey& _, const key_type* key) {
    flush();
    return _data.get<SKey>().equal_range(key);
  }
  void dump();
  std::size_t size() {
    flush();
    return _data.size();
  }

protected:
  virtual bool insertTuple(Tuple* t);
  virtual bool eraseTuple(Tuple* t, bool match_key_only);
  virtual BaseTablePtr clone();

  ptime flush();
  void flush_timercb();
  void evict();
};

SOFTTABLE_TEMPLATE
void SOFTTABLE_TEMPLATE_PREFIX::dump(){
  flush();
  std::cout << "dumping softTable.." << std::endl;
  for (typename pindex_type::iterator it=_data.get<0>().begin();it!=_data.get<0>().end();++it){
    std::cout << "  " << *(*it).tuple << "\t\t[expTime:" << (*it).exp_time << ']'<< std::endl;
  }
}

SOFTTABLE_TEMPLATE
bool SOFTTABLE_TEMPLATE_PREFIX::insertTuple(Tuple* tp){
  TupleT* t = tuple_ptr_cast<TupleT*>(tp);
  ptime t_now = flush();
  Entry e(t);
  e.exp_time = t_now;
  const key_type* key = NULL;
  key = pkey_proj_type()(t);
  std::pair<typename pindex_type::iterator, bool> r
    =_data.get<0>().insert(e);
  if (!r.second) {// existed
    if ((*(*r.first).tuple) ==*t) {
      //duplicate tuple
      //(*r.first).exp_time = t_now;
      _data.get<0>().replace(r.first, e);
      if (this->_queue) {
        Event ev(Event::REFRESHED, (*r.first).tuple);
        this->_queue->enqueue(ev);
      }
      return false;
    }else {
      if (this->_queue) {
        Event ev(Event::DELETED, (*r.first).tuple);
        this->_queue->enqueue(ev);
      }
      _data.get<0>().replace(r.first, e);
    }
  }
  //insertion succeeded
  if (this->_queue){
    Event ev(Event::INSERTED, t);
    this->_queue->enqueue(ev);
  }
  evict();
  return true;  
}

SOFTTABLE_TEMPLATE
bool SOFTTABLE_TEMPLATE_PREFIX::eraseTuple(Tuple* tp, bool match_key_only){
  TupleT* t = tuple_ptr_cast<TupleT*>(tp);
  flush();
  const key_type* key = pkey_proj_type()(t);
  typename pindex_type::iterator it;
  it = _data.get<0>().find(key);
  if (it == _data.get<0>().end()) //do not exist
    return false;
  if ( !match_key_only && (*((*it).tuple) != *t) ) // not the same tuple
    return false;
  t = it->tuple;
  _data.get<0>().erase(it);
  if (this->_queue){
    Event ev(Event::DELETED, t);
    this->_queue->enqueue(ev);
  }
  return true;
}

SOFTTABLE_TEMPLATE
BaseTablePtr SOFTTABLE_TEMPLATE_PREFIX::clone()
{
  return new SOFTTABLE_TEMPLATE_PREFIX(*this);
}

SOFTTABLE_TEMPLATE
TupleT* SOFTTABLE_TEMPLATE_PREFIX::primaryLookup(const key_type* key) {
  flush();
  typename pindex_type::const_iterator it;
  it =  _data.get<0>().find(key);
  if (it == _data.get<0>().end())
    return NULL;
  return (*it).tuple;
}

SOFTTABLE_TEMPLATE
void SOFTTABLE_TEMPLATE_PREFIX::flush_timercb(){
  //_flush_timer.cancel();
  callLater(minimum_flush_period,
            boost::bind(& SOFTTABLE_TEMPLATE_PREFIX::flush_timercb,
                        this),
            &_flush_timer);
  flush();
}

SOFTTABLE_TEMPLATE
ptime SOFTTABLE_TEMPLATE_PREFIX::flush(){
  
  ptime t0 = now();
  ptime t_cutoff = t0 - _timeout;
  exp_time_idx_type& idx = _data.get<TimeStampKey>();
  typename exp_time_idx_type::iterator it,e;
  e = idx.upper_bound(&t_cutoff);
  for (it=idx.begin();
       it!=e;
       ){
    if (this->_queue){
      Event ev(Event::EXPIRED, (*it).tuple);
      this->_queue->touchTable(this);
      this->_queue->enqueue(ev);
    }
    
    idx.erase(it++);
  }
  return t0;
}

SOFTTABLE_TEMPLATE
void SOFTTABLE_TEMPLATE_PREFIX::evict(){
  if (_maxSize==0) return; //infinity size
  exp_time_idx_type& idx = _data.get<TimeStampKey>();
  while (_data.size()> _maxSize){
    typename exp_time_idx_type::iterator it = idx.begin();
    if (this->_queue){
      this->_queue->enqueue(Event(Event::EVICTED, (*it).tuple));
    }
    idx.erase(it);
  }  
}

///////////////////////////////////////////////////////////////////


#endif
