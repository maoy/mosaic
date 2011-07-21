/* $Id: tuple_projection.hpp 211 2009-04-14 22:16:01Z maoy $ */

#ifndef _TUPLE_PROJECTION_HPP__
#define _TUPLE_PROJECTION_HPP__

#include <boost/tuple/tuple.hpp>

using boost::tuple;
template <int P1=-1, int P2=-1, int P3=-1, int P4=-1, int P5=-1>
struct Keys
{};


//forward definition
template<class TupleT, class K> struct TupleProjection;
//project a tuple to itself
template<class TupleT>
struct TupleProjection<TupleT, Keys<-1, -1, -1, -1, -1> > {
  typedef TupleT tuple_type;
  typedef tuple_type key_type;
  typedef const key_type* result_type;
  result_type
  operator()(const TupleT* t) const{
    return t;
  }
};
/* this will map 1 position to the key.
 */
template<class TupleT, int Pos1>
struct TupleProjection<TupleT, Keys<Pos1, -1, -1, -1, -1> > {

  typedef TupleT tuple_type;
#if 0
  // project the tuple to a 1 field tuple
  typedef tuple< typename boost::tuples::element<Pos1,TupleT>::type
		 > key_type;

  typedef key_type* result_type;
  result_type
  operator()(const TupleT* t) const{
    return new (UseGC) key_type( (boost::get<Pos1>)(*t) );
  }
#else
  //project the tuple to the pointer of the actual field
  typedef typename boost::tuples::element<Pos1,TupleT>::type key_type;

  typedef const key_type* result_type;
  result_type
  operator()(const TupleT* t) const{
    return & ((boost::get<Pos1>)(*t)) ;
  }
#endif
  /*
    rmap: given a key, get a tuple, with non-key fields default values (NULL, 0, etc)
   */
  static void rmap(const key_type* key, tuple_type* & t){
    t = new (UseGC) tuple_type();
    boost::get<Pos1>(*t) = boost::get<0>(*key);
  }
};


/* this will map 2 positions to the key.
 */


template<class TupleT, int Pos1, int Pos2>
struct TupleProjection<TupleT, Keys<Pos1,Pos2,-1,-1,-1> >{
  typedef TupleT tuple_type;
  typedef tuple< typename boost::tuples::element<Pos1,TupleT>::type,
		 typename boost::tuples::element<Pos2,TupleT>::type
		 > key_type;
  typedef key_type* result_type;
  static void rmap(const key_type* key, tuple_type* & t){
    t = new (UseGC) tuple_type();
    boost::get<Pos1>(*t) = boost::get<0>(*key);
    boost::get<Pos2>(*t) = boost::get<1>(*key);
  }
  result_type operator()(const tuple_type* t) const{
    return new (UseGC) key_type( boost::get<Pos1>(*t), boost::get<Pos2>(*t) );
  }
};

/* this will map 3 positions to the key.
 */
template<class TupleT, int Pos1, int Pos2, int Pos3>
struct TupleProjection<TupleT, Keys<Pos1,Pos2,Pos3,-1,-1> >{
  typedef TupleT tuple_type;
  typedef tuple< typename boost::tuples::element<Pos1,TupleT>::type,
		 typename boost::tuples::element<Pos2,TupleT>::type,
		 typename boost::tuples::element<Pos3,TupleT>::type
		 > key_type;
  typedef key_type* result_type;
  static void rmap(const key_type* key, tuple_type* & t){
    t = new (UseGC) tuple_type();
    boost::get<Pos1>(*t) = boost::get<0>(*key);
    boost::get<Pos2>(*t) = boost::get<1>(*key);
    boost::get<Pos3>(*t) = boost::get<2>(*key);
  }
  result_type operator()(const tuple_type* t) const{
    return new (UseGC) key_type( boost::get<Pos1>(*t), boost::get<Pos2>(*t), boost::get<Pos3>(*t) );
  }
};

/* this will map 4 positions to the key.
 */
template<class TupleT, int Pos1, int Pos2, int Pos3, int Pos4>
struct TupleProjection<TupleT, Keys<Pos1,Pos2,Pos3,Pos4,-1> >{
  typedef TupleT tuple_type;
  typedef tuple< typename boost::tuples::element<Pos1,TupleT>::type,
		 typename boost::tuples::element<Pos2,TupleT>::type,
		 typename boost::tuples::element<Pos3,TupleT>::type,
		 typename boost::tuples::element<Pos4,TupleT>::type
		 > key_type;
  typedef key_type* result_type;
  static void rmap(const key_type* key, tuple_type* & t){
    t = new (UseGC) tuple_type();
    boost::get<Pos1>(*t) = boost::get<0>(*key);
    boost::get<Pos2>(*t) = boost::get<1>(*key);
    boost::get<Pos3>(*t) = boost::get<2>(*key);
    boost::get<Pos4>(*t) = boost::get<3>(*key);
  }
  result_type operator()(const tuple_type* t) const{
    return new (UseGC) key_type( boost::get<Pos1>(*t), boost::get<Pos2>(*t), boost::get<Pos3>(*t), boost::get<Pos4>(*t) );
  }
};


/* this will map 5 positions to the key.
 */
template<class TupleT, int Pos1, int Pos2, int Pos3, int Pos4, int Pos5>
struct TupleProjection<TupleT, Keys<Pos1,Pos2,Pos3,Pos4,Pos5> >{
  typedef TupleT tuple_type;
  typedef tuple< typename boost::tuples::element<Pos1,TupleT>::type,
		 typename boost::tuples::element<Pos2,TupleT>::type,
		 typename boost::tuples::element<Pos3,TupleT>::type,
		 typename boost::tuples::element<Pos4,TupleT>::type,
		 typename boost::tuples::element<Pos5,TupleT>::type
		 > key_type;
  typedef key_type* result_type;
  static void rmap(const key_type* key, tuple_type* & t){
    t = new (UseGC) tuple_type();
    boost::get<Pos1>(*t) = boost::get<0>(*key);
    boost::get<Pos2>(*t) = boost::get<1>(*key);
    boost::get<Pos3>(*t) = boost::get<2>(*key);
    boost::get<Pos4>(*t) = boost::get<3>(*key);
    boost::get<Pos5>(*t) = boost::get<4>(*key);
  }
  result_type operator()(const tuple_type* t) const{
    return new (UseGC) key_type( boost::get<Pos1>(*t), boost::get<Pos2>(*t)
                                 , boost::get<Pos3>(*t), boost::get<Pos4>(*t), boost::get<Pos5>(*t) );
  }
};


#endif
