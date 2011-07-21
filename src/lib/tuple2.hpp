/* $Id: tuple2.hpp 211 2009-04-14 22:16:01Z maoy $ */

#ifndef _TUPLE2_HPP__
#define _TUPLE2_HPP__

#include <boost/static_assert.hpp>
/* do not include this file directly, include tuple.hpp instead.
 */
using boost::tuple;
template <class T, int ID>
class Tuple2: public T, public Tuple {
public:
  typedef T tuple_type;
  typedef Tuple2<T,ID> my_type;
  enum { TypeID = ID };
  typedef Tuple base_type;

  //constructors
  Tuple2(): tuple_type(), Tuple("tuple2", ID) {}
  template <class T1>
  Tuple2(const T1 & t1)
    :tuple_type(t1), 
     Tuple("tuple2", ID)
  {BOOST_STATIC_ASSERT(boost::tuples::length<T>::value == 1);}
  template <class T1, class T2>
  Tuple2(const T1 & t1, const T2& t2)
    :tuple_type(t1,t2), 
     Tuple("tuple2", ID)
  {BOOST_STATIC_ASSERT(boost::tuples::length<T>::value == 2);}
  template <class T1, class T2, class T3>
  Tuple2(const T1 & t1, const T2& t2, const T3& t3)
    :tuple_type(t1,t2,t3), 
     Tuple("tuple2", ID)
  {BOOST_STATIC_ASSERT(boost::tuples::length<T>::value == 3);}
  template <class T1, class T2, class T3, class T4>
  Tuple2(const T1 & t1, const T2& t2, const T3& t3, const T4& t4)
    :tuple_type(t1,t2,t3,t4), 
     Tuple("tuple2", ID)
  {BOOST_STATIC_ASSERT(boost::tuples::length<T>::value == 4);}
  template <class T1, class T2, class T3, class T4, class T5>
  Tuple2(const T1 & t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5)
    :tuple_type(t1,t2,t3,t4,t5), 
     Tuple("tuple2", ID)
  {BOOST_STATIC_ASSERT(boost::tuples::length<T>::value == 5);}
  template <class T1, class T2, class T3, class T4, class T5, class T6>
  Tuple2(const T1 & t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6)
    :tuple_type(t1,t2,t3,t4,t5,t6), 
     Tuple("tuple2", ID)
  {BOOST_STATIC_ASSERT(boost::tuples::length<T>::value == 6);}
  template <class T1, class T2, class T3, class T4, class T5, class T6, class T7>
  Tuple2(const T1 & t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7)
    :tuple_type(t1,t2,t3,t4,t5,t6,t7), 
     Tuple("tuple2", ID)
  {BOOST_STATIC_ASSERT(boost::tuples::length<T>::value == 7);}
  template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
  Tuple2(const T1 & t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7, const T8& t8)
    :tuple_type(t1,t2,t3,t4,t5,t6,t7,t8), 
     Tuple("tuple2", ID)
  {BOOST_STATIC_ASSERT(boost::tuples::length<T>::value == 8);}
  template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9>
  Tuple2(const T1 & t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7, const T8& t8, const T9& t9)
    :tuple_type(t1,t2,t3,t4,t5,t6,t7,t8,t9),
     Tuple("tuple2", ID)
  {BOOST_STATIC_ASSERT(boost::tuples::length<T>::value == 9);}
  virtual ~Tuple2() {}
  ////////////////
  virtual std::string toString() const {
    std::ostringstream oss;
    oss << *this;
    return oss.str();
  }
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version){
    serialize_tuple(ar, static_cast<tuple_type*>(this) );
  }
};

#define DEFINE_TUPLE(name)                                      \
  const char* const_str_##name = #name;                                 \
  typedef Tuple2< type_tuple_##name, ID_Tuple_##name> Tuple_##name;     \
  inline std::ostream& operator<<(std::ostream& os, const Tuple_##name& n) \
  {									\
    return os << const_str_##name                                       \
              << static_cast<type_tuple_##name>(n);                     \
  }									\
  inline std::ostream& operator<<(std::ostream& os, const Tuple_##name* n) \
  {									\
    return os << "*" << *n;						\
  }

#endif

