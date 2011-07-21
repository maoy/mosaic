/* $Id: packet.cpp 210 2009-04-14 22:14:22Z maoy $ */

#include "packet.hpp"

template <>
void Packet::load(NetIArchive & ia, const unsigned int version)
{
  length_t len;
  assert( !_buf );
  ia & len;
  if ( len != (ia._buf)->size() ) {
    std::cerr << "packet len=" << len <<" but buf size=" << ia._buf->size() << std::endl;
    throw std::range_error("packet length mismatch buffer size?!");
  }else{
    _buf = ia._buf;
  }    
}
