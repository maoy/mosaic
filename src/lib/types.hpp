/* $Id: types.hpp 211 2009-04-14 22:16:01Z maoy $ */

#ifndef TYPES_HPP_
#define TYPES_HPP_

#include <map>
#include <vector>
#include <iostream>
#include <sstream>
#include <string>

#include <boost/assert.hpp>
#include <boost/serialization/nvp.hpp>
using boost::serialization::make_nvp;
#include <boost/serialization/split_member.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>

#include <gc/gc_cpp.h>
#include <gc/gc_allocator.h>


#include "new_streambuf.hpp"

typedef uint16_t length_t;
typedef uint16_t typeid_t;
typedef std::basic_string<char,std::char_traits<char>, gc_allocator<char> > _gc_string;
typedef _gc_string gc_string;
typedef std::vector<gc_string, gc_allocator<gc_string> > string_vector;


template <class T>
T from_string_cast(const char*);

template <class T>
T from_string_cast(const char* s)
{
  throw std::runtime_error(std::string("unknown type when casting from string:")+std::string(s) );
}

#define RAW_CAST(t) template <>                         \
  inline t from_string_cast(const char* s)    \
  {                                                     \
    return boost::lexical_cast<t>(s);                   \
  }

RAW_CAST(int8_t)
RAW_CAST(int16_t)
RAW_CAST(int32_t)
RAW_CAST(int64_t)
RAW_CAST(uint8_t)
RAW_CAST(uint16_t)
RAW_CAST(uint32_t)
RAW_CAST(uint64_t)
RAW_CAST(double)
RAW_CAST(float)
template<>
inline gc_string from_string_cast(const char* s)
{
  return gc_string(s);
}

template <class T> struct ptrLess
  :public std::binary_function <const T*,const T*,bool> {
  bool operator() (const T* x, const T* y) const
  {return *x<*y;}
};

template <class T> struct ptrEqual
  :public std::binary_function <const T*,const T*,bool> {
  bool operator() (const T* x, const T* y) const
  {return *x==*y;}
};

class TypeInfo
{
public:
  const typeid_t type_id;
  const char* type_name;
  explicit TypeInfo(const typeid_t id, const char* n)
    : type_id(id), type_name(n)
  {}
};
class Object{
protected:
  void *_p;
public:
  const int id;
  Object(int i, void* p) :_p(p), id(id){}
  virtual ~Object(){}
};
class ICommitable{
public:
  virtual bool commit()=0;
  virtual ~ICommitable(){}
};


typedef new_streambuf<gc_allocator<char> > gc_streambuf;


////////////////////
template <class T>
class gc_ptr{
public:
  typedef gc_ptr<T> mytype;

  //gc_ptr():p(NULL){}
  gc_ptr(const T& t):p(new (UseGC) T(t)){}
  gc_ptr(T* t=NULL):p(t) {}
  gc_ptr(const mytype& m):
    p(m.p){}
  mytype& operator=(mytype const& m){
    p = m.p; return *this;
  }
  mytype copy() const{
    return mytype(*p);
  }
  ~gc_ptr(){}

  mytype operator+(const mytype & rhs) const{
    //BOOST_ASSERT(p && rhs.p);
    if ( (!p) || (!rhs.p) ) //anything+ NULL = NULL
      return mytype();
    return mytype((*p) + (*rhs.p));
  }
  mytype operator-(const mytype & rhs) const{
    //BOOST_ASSERT(p && rhs.p);
    if ( (!p) || (!rhs.p) ) //anything - NULL = NULL
      return mytype();
    return mytype((*p) - (*rhs.p));
  }
  mytype operator/(const mytype & rhs) const{
    //BOOST_ASSERT(p && rhs.p);
    if ( (!p) || (!rhs.p) ) //anything * NULL = NULL
      return mytype();
    return mytype((*p) / (*rhs.p));
  }
  mytype operator*(const mytype & rhs) const{
    //BOOST_ASSERT(p && rhs.p);
    if ( (!p) || (!rhs.p) ) //anything / NULL = NULL
      return mytype();
    return mytype((*p) * (*rhs.p));
  }
  const T& get() const{
    BOOST_ASSERT(p);
    return *p;
  }
  T& nonconst_get() const{
    BOOST_ASSERT(p);
    return *p;
  }
  T* get_ptr() const{
    return p;
  }
  const bool isNull() const{
    return (!p);
  }
  std::string toString() const{
    std::ostringstream oss;
    oss  << (*this);
    return oss.str();
  }
  template<class Archive>
  void save(Archive & ar, const unsigned int version) const
  {
    bool flag = isNull();
    ar & make_nvp("isNull",flag);
    if (!flag)
      ar & make_nvp("value",*static_cast<const T*>(p) );
  }
  template<class Archive>
  void load(Archive & ar, const unsigned int version)
  {
    bool flag;
    ar & flag;
    if (flag)
      p = NULL;
    else{
      p = new (UseGC) T();
      ar & *p;
    }
  }
  BOOST_SERIALIZATION_SPLIT_MEMBER()

  //allow placement new
  void* operator new( size_t size , void* p){
    return p;
  }
protected:
  T* p;

private:

  //Do not allowed to be allocated on heap. shouldn't be called and not implemented
  void* operator new( size_t size );
};
template <class T>
std::ostream & operator<< (std::ostream & os, gc_ptr<T> const & p){
  if ( p.isNull() )
    os << "NULL";
  else
    os << p.get();
  return os;
}

template <class U, class T>
bool operator==(gc_ptr<U> const& lhs, gc_ptr<T> const& rhs){
  if (lhs.isNull()){
    // NULL == NULL ?
    return (rhs.isNull())?true:false;
  }
  if (rhs.isNull()){
    // not NULL != NULL
    return false;
  }
  return (lhs.get()) == (rhs.get());
}

template <class U>
bool operator==(gc_ptr<U> const& lhs, U const& rhs){
  if (lhs.isNull()){
    // NULL ?
    return false;
  }
  return (lhs.get() == rhs);
}

template <class U>
bool operator==(U const& lhs, gc_ptr<U> const& rhs){
  return rhs==lhs;
}

template <class U, class T>
inline bool operator!=(gc_ptr<U> const& lhs, gc_ptr<T> const& rhs){
  return !(lhs==rhs);
}
template <class U, class T>
inline bool operator<(gc_ptr<U> const& lhs, gc_ptr<T> const& rhs){
  if (lhs.isNull()){
    //NULL > not NULL
    return false;
  }
  if (rhs.isNull()) return true; // not NULL < NULL
  return (lhs.get()) < (rhs.get());
}

template <class U, class T>
bool operator>(gc_ptr<U> const& lhs, gc_ptr<T> const& rhs){
  if (lhs.isNull()){
    //NULL > not NULL
    return (rhs.isNull())?false:true;
  }
  if (rhs.isNull())  return false;
  return (lhs.get()) > (rhs.get());
}

template <class U, class T>
inline bool operator<=(gc_ptr<U> const& lhs, gc_ptr<T> const& rhs){
  return !(lhs > rhs);
}

template <class U, class T>
inline bool operator>=(gc_ptr<U> const& lhs, gc_ptr<T> const& rhs){
  return !(lhs<rhs);
}


///////////////////////////
/* No lock singlton*/
template <class T>
class Singleton
{
public:
  inline static T* getInstance(){
    static T instance;
    return & instance;
  }
protected:
  Singleton(){}
private:
  //no implementation
  Singleton(const Singleton&);
  Singleton& operator=(const Singleton&);
};
////////////////////////////
typedef gc_ptr<int> Integer;
typedef gc_ptr<double> Double;
typedef gc_ptr<gc_string> String;

#endif
