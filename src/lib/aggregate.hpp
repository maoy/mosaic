/* $Id: aggregate.hpp 211 2009-04-14 22:16:01Z maoy $ */

#ifndef _AGGREGATE_HPP
#define _AGGREGATE_HPP

#include "table.hpp"

/*
  Given an iterator of Entry*, use Extract to get the field
  to be aggregated, and return the minimum value.
 */

template<class Extract, class IT>
typename IT::value_type a_min_entry(IT from, IT to, Extract ex){
  typename IT::value_type result;
  if (from==to) // empty?
    return result;
  else{
    result = (*from);
  }
  
  for (IT it= (++from); it!=to; ++it){
    typename Extract::result_type s = ex(*it);
    if ( s<ex(result) )
      result = *it;
  }
  return result;
}
template<class Extract, class IT>
typename Extract::result_type a_min(IT from, IT to, Extract ex){
  typename IT::value_type e= a_min_entry(from, to, ex);
  typename Extract::result_type r;
  //if (e)
  r = ex(e);
  return r;
}


template<class Extract, class IT>
typename Extract::tuple_type* a_min_tuple(IT from, IT to, Extract ex){
  typename IT::value_type e= a_min_entry(from, to, ex);
  typename Extract::tuple_type* r = NULL;
  r = e.tuple;
  return r;
}
/* end of min related */
template<class Extract, class IT>
typename IT::value_type a_max_entry(IT from, IT to, Extract ex){
  typename IT::value_type result;
  if (from==to) // empty?
    return result;
  else{
    result = (*from);
  }
  
  for (IT it= (++from); it!=to; ++it){
    typename Extract::result_type s = ex(*it);
    if ( s>ex(result) )
      result = *it;
  }
  return result;
}
template<class Extract, class IT>
typename Extract::result_type a_max(IT from, IT to, Extract ex){
  typename IT::value_type e= a_max_entry(from, to, ex);
  typename Extract::result_type r;
  //if (e)
  r = ex(e);
  return r;
}

template<class Extract, class IT>
typename Extract::tuple_type* a_max_tuple(IT from, IT to, Extract ex){
  typename IT::value_type e= a_max_entry(from, to, ex);
  typename Extract::tuple_type* r = NULL;
  r = e.tuple;
  return r;
}


template<class Extract, class IT>
int a_count(IT from, IT to, Extract ex){
  int result = 0;
    
  for (IT it= from; it!=to; ++it){
    //typename Extract::result_type s = ex(*it);
    result++;
  }
  return result;
}

template<class Extract, class IT>
Double a_average(IT from, IT to, Extract ex){
  int count = 0;
  //typename Extract::result_type sum = 0;
  double sum = 0.0;
  for (IT it= from; it!=to; ++it){
    //typename Extract::result_type s = ex(*it);
    sum += ex(*it);
    count++;
  }
  Double res;
  if (count==0) return res; // return NULL
  return sum/count;
}

template<class Extract, class IT>
typename Extract::result_type a_sum(IT from, IT to, Extract ex){
  typename Extract::result_type sum = 0;
  for (IT it= from; it!=to; ++it){
    sum += ex(*it);
  }
  return sum;
}

template<class STable, class GroupKey, int Pos>
struct Aggregate{
  typedef typename STable::tuple_type tuple_type;
  typedef FieldExtraction<typename STable::entry_type, Pos> extraction_type;
  typedef typename extraction_type::result_type position_type; //type of tuple[Pos]
  typedef TupleProjection<tuple_type,GroupKey> gkey_proj_type;//group by mapping
  typedef typename gkey_proj_type::key_type gkey_type;
  typedef typename STable::container_type::template index<GroupKey>::type::iterator iter_type;
  typedef std::pair<iter_type,iter_type> iter_pair_type;
  STable* _tbl;
  extraction_type _ext;
  gkey_proj_type _proj;
  iter_type from, to;
  GroupKey _gk;
  explicit Aggregate(STable* tbl, tuple_type*t): _tbl(tbl){
    tie(from, to) = _tbl->secondaryLookup(_gk, _proj(t) );
  }
  explicit Aggregate(STable* tbl, gkey_type*k): _tbl(tbl){
    tie(from, to) = _tbl->secondaryLookup(_gk, k );
  }
  ~Aggregate(){}
  position_type min(){
    return a_min(from, to, _ext);
  }
  tuple_type* min_tuple(){
    return a_min_tuple(from, to, _ext);
  }
  position_type max(){
    return a_max(from, to, _ext);
  }
  tuple_type* max_tuple(){
    return a_max_tuple(from, to, _ext);
  }
  int count(){
    return a_count(from, to, _ext);
  }
  int average(){
    return a_average(from, to, _ext);
  }
};

/*
template<class STable, class GroupKey, int Pos>
struct AggTable{
  STable* _tbl;
  struct iterator{
    typedef typename STable::container_type::template index<GroupKey>::type::iterator iter_type;
    typedef Aggregate<STable, GroupKey, Pos> value_type;
    typedef typename iter_type::value_type entry_type;
    typedef EntryProjection<entry_type,GroupKey> proj_type;
    proj_type _proj;
    iter_type _internal_it;
    STable* _tbl;
    explicit iterator(const iter_type& it, STable* tbl): _internal_it(it), _tbl(tbl){}
    iterator& operator++(){
      iter_type from, to;
      tie(from,to)=_tbl->_data.get<GroupKey>().equal_range( _proj(*_internal_it) );
      _internal_it = to;
      return *this;
    }
    value_type operator*(){
      return Aggregate<STable,GroupKey,Pos>(_tbl, _proj(*_internal_it) );
    }
    bool operator==(const iterator& it){
      return _internal_it == it._internal_it;
    }
    bool operator!=(const iterator& it){
      return _internal_it != it._internal_it;
    }
  };
  explicit AggTable(STable* tbl): _tbl(tbl){
  }
  iterator begin() const{
    return iterator( _tbl->_data.get<GroupKey>().begin(),_tbl );
  }
  iterator end() const{
    return iterator( _tbl->_data.get<GroupKey>().end(), _tbl );
  }
};
*/

#endif
