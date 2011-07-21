/* $Id: poly_ptr.hpp 211 2009-04-14 22:16:01Z maoy $ */

#ifndef _POLY_HPP_
#define _POLY_HPP_

#include "types.hpp"
#include "s11n.hpp"

template <class BaseT>
class poly_ptr: public gc_ptr<BaseT>{
public:
  poly_ptr(BaseT* t= NULL): gc_ptr<BaseT>(t){}
  
  template<class Archive>
  void save(Archive & ar, const unsigned int version) const
  {
    typeid_t tid = this->isNull()? 0: this->get().type_id;
    ar & tid;
    if (tid)
      SavePointerHelper<Archive,BaseT>::getInstance()->save(tid, static_cast<BaseT*>(this->p), ar);
  }
  template<class Archive>
  void load(Archive & ar, const unsigned int version)
  {
    typeid_t tid;
    ar & tid;
    if (!tid)
      this->p = NULL;
    else{
      this->p = LoadPointerHelper<Archive,BaseT>::getInstance()->load(tid,ar);
    }
  }
  BOOST_SERIALIZATION_SPLIT_MEMBER()  
  
};

#endif

