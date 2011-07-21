/* $Id: udp.hpp 225 2009-06-16 20:49:02Z maoy $ */


#ifndef __UDP_HPP__
#define __UDP_HPP__

#include <string>
using std::string;

//#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>
//#include <boost/archive/binary_oarchive.hpp>

#include "types.hpp"
#include "s11n.hpp"
#include "tuple.hpp"
#include "runtime.hpp"

#include "address.hpp"

//typedef boost::function<void (void)>        b_cbv;
class UdpSocket {
public:
  UdpSocket(const gc_string& name, uint16_t port, boost::asio::io_service* io=&iosv)
    :_name(name), _port(port), _io(io), _buf(NULL),  _skt(NULL)
  {}
  int init(); //this will create the socket and bind it to port
  
  ~UdpSocket(){};
  //void sendTo(const char* buf, std::size_t size, const string& addr);
  template<class T>
  void sendTo(const T*, const gc_string& addr);
  template<class T>
  void sendTo(const T*, const UDPAddress& addr);
  void enableRead();
private:
  void onRead(const boost::system::error_code& error,
                      std::size_t /*bytes_transferred*/);
  //void onWrite(){};
  //void onError(){};

  gc_string _name;
  uint16_t _port;
  boost::asio::io_service* _io;
  boost::asio::ip::udp::endpoint _remote_endpoint;
  gc_streambuf* _buf;
public:
  boost::asio::ip::udp::socket*  _skt;
};


template<class T>
void UdpSocket::sendTo(const T* t, const gc_string& addr){
  using namespace boost::asio;
  using ip::udp;
  //udp::resolver resolver(*_io);
  gc_string ip; uint16_t port;
  if ( parseAddr(addr.c_str(), ip, port)<0) return;
  sendTo(t, UDPAddress(ip.c_str(), port) );
}

template<class T>
void UdpSocket::sendTo(const T* t, const UDPAddress& addr){
  using namespace boost::asio;
  using ip::udp;
  udp::endpoint remote_endpoint(
                                //ip::address::from_string(addr._addr.toString()),
                                ip::address_v4( addr._addr.toHost() ),
                                addr._port);
  NetOArchive oa;
  //Tuple::toBuffer(t, oa);
  oa & magic_number;
  oa & t->type_id;
  oa & *t;
  std::size_t sent_len = _skt->send_to(*oa.data(), remote_endpoint);
  //std::cout << "udp sent len: " << sent_len << std::endl;
  (void)sent_len;
}

///////////////////////////////////////

class UdpManager:public Singleton<UdpManager> {
private:
  UdpSocket* skt_; // each node only has 1 udp socket listening
public:
  void listen(const UDPAddress addr);
  //given the destination address, find the right socket to send to
  UdpSocket* findSocket(const UDPAddress& dest);
  //unfinished
};
#endif /* __UDP_HPP__ */
