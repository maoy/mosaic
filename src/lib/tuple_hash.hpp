/* $Id: tuple_hash.hpp 211 2009-04-14 22:16:01Z maoy $ */

#ifndef TUPLE_HASH_HPP_
#define TUPLE_HASH_HPP_
//for tuple hashing
#include <boost/fusion/algorithm/iteration/fold.hpp>
#include <boost/fusion/adapted/boost_tuple.hpp>
#include <boost/functional/hash.hpp> 

//class intended for internal use by TupleHash
struct hash_combine_instance {
  typedef std::size_t result_type;
  template<class T>
  std::size_t operator()(const T& arg, std::size_t current) const{
    boost::hash_combine(current, arg);
    return current;
  }
}; 

// a functor to hash a tuple pointer
template<class T>
struct TupleHash {
  typedef T tuple_type;
  std::size_t operator()(const T* t) const {
    return boost::fusion::fold(*t, 0, hash_combine_instance() ); 
  }   
};

#endif
