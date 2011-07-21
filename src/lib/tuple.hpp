/* $Id: tuple.hpp 211 2009-04-14 22:16:01Z maoy $ */

#ifndef __TUPLE_HPP__
#define __TUPLE_HPP__

#include <string>
#include <sstream>
#include <cstddef> 
#include <iostream>
#include <list>
#include <gc/gc_cpp.h>
#include <gc/gc_allocator.h>

#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>
#include <boost/tuple/tuple_io.hpp>

#include "poly_ptr.hpp"
#include "s11n.hpp"
#include "tuple_projection.hpp"

using boost::tuple;

template<class tuple_type,int N>
struct do_serialize {
  template <class Archive>
  inline static void call(Archive& ar, tuple_type *t) {
    do_serialize<tuple_type, N-1>::call(ar,t);
    ar & boost::get<N>(*t);
  }
};

template<class tuple_type>
struct do_serialize<tuple_type, 0> {
  template <class Archive>
  inline static void call(Archive &ar, tuple_type *t) {
    ar & boost::get<0>(*t);
  }
};

template<class Archive, class tuple_type>
inline void serialize_tuple(Archive & ar, tuple_type *t)
{
  do_serialize<tuple_type, 
    boost::tuples::length<tuple_type>::value -1 >::call(ar,t);
} 

enum TupleID {
  TUPLE_RESERVED=100,
};

const static uint16_t magic_number = 135;
const static char* raw_tuple_name = "raw_tuple";
class Tuple: public gc, public TypeInfo
{
public:
  Tuple(const char* name = raw_tuple_name, typeid_t tid=TUPLE_RESERVED):
    TypeInfo(tid, name)
  {
  }
  virtual std::string toString() const=0;
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version){
    throw std::logic_error("cannot serialize a virtual class");
  }
  template <class T, class OArchive>
  static void toBuffer(const T* tuple,
		       OArchive& );
  template <class IArchive>
  static Tuple* fromBuffer(IArchive& buf);
  virtual ~Tuple(){}
};

inline std::ostream& operator<<(std::ostream& os, const Tuple& n)
{								
  return os << n.toString();
}

template <class T, class OArchive>
void Tuple::toBuffer(const T* tuple, 
		     OArchive& oa){
  oa & magic_number;
  oa & tuple->type_id;
  oa & *tuple;
  return;
}
template <class IArchive>
Tuple* Tuple::fromBuffer(IArchive& ia){
  uint16_t mn;
  ia & mn;
  if (mn != magic_number){
    throw std::runtime_error("magic number mistmatch");
  }
  poly_ptr<Tuple> ptr;
  ia & ptr;
  return ptr.get_ptr();
}

#include "tuple2.hpp"
typedef std::list<Tuple*, gc_allocator<Tuple*> > TupleList;

template<class T>
T tuple_ptr_cast(Tuple* t){
#if 0
  return dynamic_cast<T>(t);
#else
  return static_cast<T>(t);
#endif
}
//////////////////////
#define REGISTER_TUPLE_ARCHIVE(name, IArchive, OArchive)        \
  LoadPointerHelper<IArchive,Tuple>::getInstance()              \
    ->registerClass<Tuple_##name>();                            \
  SavePointerHelper<OArchive,Tuple>::getInstance()              \
    ->registerClass<Tuple_##name>();

////////////////////
#endif
