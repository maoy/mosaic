/* $Id: vector.hpp 211 2009-04-14 22:16:01Z maoy $ */

#ifndef VECTOR_HPP_
#define VECTOR_HPP_
#include <deque>
#include <algorithm>

#include <boost/foreach.hpp>
#include <boost/serialization/deque.hpp>
#include "types.hpp"

namespace mosaic{

template <class T>
class Vector: public gc_ptr<std::deque<T, gc_allocator<T> > >{
public:
  typedef std::deque<T, gc_allocator<T> > vector_type;
  Vector(vector_type* t=NULL):gc_ptr<vector_type>(t) {}

  //concat with another vector
  Vector operator+(const Vector& rhs) const;
  //check if an element exist
  bool find(const T& p) const{
    return std::find(this->get().begin(),
                     this->get().end(), 
                     p) != this->get().end();
  }
};
template <class T>
Vector<T> Vector<T>::operator+(const Vector<T>& rhs) const {
    vector_type* r = new (UseGC) vector_type(this->get());
    for (typename vector_type::const_iterator it=rhs.get().begin();
	 it!= rhs.get().end();
	 ++it){
      r->push_back(*it);
    }
    return Vector(r);
  }

template <class T>
Vector<T> f_push_back(const Vector<T>& vec, const T& p){
  typename Vector<T>::vector_type* r
    =new (UseGC) typename Vector<T>::vector_type(vec.get());
  r->push_back(p);
  return Vector<T>(r);
}

template <class T>
Vector<T> f_push_front(const Vector<T>& vec, const T& p){
  typename Vector<T>::vector_type* r
    =new (UseGC) typename Vector<T>::vector_type(vec.get());
  r->push_front(p);
  return Vector<T>(r);
}

template <class T>
bool f_is_member(const Vector<T>& vec, const T& p){
  return std::find(vec.get().begin(),
		   vec.get().end(), 
		   p) != vec.get().end();
}

//return a vector with 1 element in
template <class T>
Vector<T> f_make_vector(const T& p){
  typename Vector<T>::vector_type* r
    =new (UseGC) typename Vector<T>::vector_type();
  r->push_back(p);
  return Vector<T>(r);
}

} //end of namespace

using namespace mosaic;
template <class T>
std::ostream & operator<< (std::ostream & os, std::deque<T,gc_allocator<T> > const & v){
  os << "[ ";
  BOOST_FOREACH(T const& x, v){
    os << x << " ";
  }
  os << "]";
  return os;
}

#endif
