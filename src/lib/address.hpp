/* $Id: address.hpp 225 2009-06-16 20:49:02Z maoy $ */

#ifndef _ADDRESS_HPP__
#define _ADDRESS_HPP__

#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>

#include <boost/tuple/tuple.hpp>

#include "types.hpp"

/* IPv4 Address stored in network order*/
class IPv4Address{
public:
  struct in_addr _addr;
  
  IPv4Address(){_addr.s_addr = 0;}
  IPv4Address(const char* addr){
    inet_aton(addr, & _addr);
  }
  IPv4Address(const gc_string& addr){
    *this = IPv4Address(addr.c_str());
  }
  IPv4Address(const IPv4Address& a){
    this->_addr = a._addr;
  }
  const char* toString(){
    return inet_ntoa(_addr);
  }
  uint32_t toHost(){
    return ntohl(_addr.s_addr);
  }
  bool operator<(const IPv4Address& rhs) const{
    return _addr.s_addr < rhs._addr.s_addr;
  }
  bool operator==(const IPv4Address& rhs) const{
    return _addr.s_addr == rhs._addr.s_addr;
  }
  bool operator!=(const IPv4Address& rhs) const{
    return _addr.s_addr != rhs._addr.s_addr;
  }
  bool operator>(const IPv4Address& rhs) const{
    return _addr.s_addr > rhs._addr.s_addr;
  }
  template<class Archive>
  void serialize(Archive & ar, const unsigned int version)
  {
    ar & _addr.s_addr;
  }
};

enum {
  ADDRESS_UDP = 0,
  ADDRESS_TCP = 1,
  ADDRESS_RESERVED=100
};
class Address: public gc
{
public:
  const uint16_t id_;
  const char*  name_;
  Address(const char* name = "addr", uint16_t aid = ADDRESS_RESERVED):
    id_(aid), name_(name)
  {}
  virtual bool isLocalAddress() const = 0;
  virtual ~Address(){}
};

int parseAddr(const char* addr, gc_string& ip, uint16_t& port);

static const char* address_const[2] = { "udp", "tcp" };
template <int Type>
class TransportAddress: public Address
                      , public boost::tuple<IPv4Address, uint16_t>
{
public:
  IPv4Address& _addr;
  uint16_t& _port;
  
  TransportAddress()
    :Address(address_const[Type], Type), 
     boost::tuple<IPv4Address,uint16_t>(),
     _addr( get<0>() ),
     _port( get<1>() )
  {
  }
  TransportAddress(const char* addr, uint16_t port)
    :Address(address_const[Type], Type), 
     boost::tuple<IPv4Address,uint16_t>(IPv4Address(addr), port),
     _addr( get<0>() ),
     _port( get<1>() )
  {
  }
  TransportAddress(const gc_string& addr, uint16_t port)
    :Address(address_const[Type], Type), 
     boost::tuple<IPv4Address,uint16_t>(IPv4Address(addr), port),
     _addr( get<0>() ),
     _port( get<1>() )
  {
  }

  bool isLocalAddress() const {
    return false; //FIXME
  }
  static TransportAddress<Type> fromString(const gc_string& addr_port)
  {
    gc_string ip; uint16_t port=0;
    parseAddr(addr_port.c_str(), ip, port);
    return TransportAddress<Type>(ip,port);
  }
  static TransportAddress<Type> fromString(const char* addr_port)
  {
    gc_string ip; uint16_t port=0;
    parseAddr(addr_port, ip, port);
    return TransportAddress<Type>(ip,port);
  }
  TransportAddress& operator=(const TransportAddress& rhs){
    _addr = rhs._addr;
    _port = rhs._port;
    return *this;
  }
  template <class Archive>
  void serialize(Archive & ar, const unsigned int version)
  {
    ar & _addr;
    ar & _port;
  }
  
};

typedef TransportAddress<ADDRESS_UDP> UDPAddress;
typedef TransportAddress<ADDRESS_TCP> TCPAddress;

inline std::ostream& operator<<(std::ostream& os, const IPv4Address& addr){
  return os << inet_ntoa(addr._addr);
}

template <int Type>
inline std::ostream& operator<<(std::ostream& os, const TransportAddress<Type>& addr){
  return os << addr.name_ << "::(" << addr._addr 
            << ":" << addr._port << ")";
}

template <>
inline UDPAddress from_string_cast(const char* s)
{
  return UDPAddress::fromString(s);
}

template <>
inline TCPAddress from_string_cast(const char* s)
{
  return TCPAddress::fromString(s);
}

#endif
