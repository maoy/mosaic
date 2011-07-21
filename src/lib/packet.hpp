/* $Id: packet.hpp 211 2009-04-14 22:16:01Z maoy $ */

#ifndef __PACKET_HPP__
#define __PACKET_HPP__

#include <ostream>
#include <arpa/inet.h>

#include "types.hpp"
#include "s11n.hpp"
#include "ip.h"


class Packet{
public:
  explicit Packet(gc_streambuf* buf = NULL):
    _buf(buf)
  {
    /*if (! _buf)
      _buf = new (UseGC) gc_streambuf;
    */
  }
  template<class Archive>
  void save(Archive & ar, const unsigned int version) const
  {
    assert(_buf);
    ar & *_buf;
  }
  template<class Archive>
  void load(Archive & ia, const unsigned int version)
  {
    throw std::logic_error("load packet: not implemented for general serialization case");
  }
  BOOST_SERIALIZATION_SPLIT_MEMBER()
  /*
  template<class Archive>
  void serialize(Archive & ar, const unsigned int version)
  {
    ar & *_buf;
  }
  */
  gc_string dstStrIP(){
    const unsigned char* b = boost::asio::buffer_cast<const unsigned char*>(_buf->data());
    const struct click_ip* header = reinterpret_cast<const struct click_ip*>(b);
    return gc_string(inet_ntoa(header->ip_dst));
  }
  uint32_t dstIP(){
    const unsigned char* b = boost::asio::buffer_cast<const unsigned char*>(_buf->data());
    const struct click_ip* header = reinterpret_cast<const struct click_ip*>(b);
    return (header->ip_dst).s_addr;
  }

  gc_streambuf* _buf;
  
};

template <>
void Packet::load(NetIArchive & ia, const unsigned int version);

inline std::ostream& operator<<(std::ostream& os, const Packet& n){
  //return os << " a packet ";
  std::size_t len = n._buf->size();
  const unsigned char* b = boost::asio::buffer_cast<const unsigned char*>(n._buf->data());
  const struct click_ip* header = reinterpret_cast<const struct click_ip*>(b);
  os << "Packet[len=" 
     << len
     << "; src=" 
     << inet_ntoa(header->ip_src)
     << "; dest=";
  os << inet_ntoa(header->ip_dst)
     << "; payload=omitted]";
  return os;
}

#endif
