/* $Id: tun.hpp 211 2009-04-14 22:16:01Z maoy $ */

#ifndef _TUN_HPP__
#define _TUN_HPP__

#include <arpa/inet.h>

#include "types.hpp"
#include "runtime.hpp"
#include "packet.hpp"
#include "tuple.hpp"

const int DEFAULT_MTU = 1500;

class TunDevice {
public:
  TunDevice(): _fd_raw(-1),
               // _mtu_in(-1), _mtu_out(-1),
               _dev_name(), _type(UNDEFINED), 
               //_near(), _mask(),
               _if_cfg(),
               _fd(iosv), _buf(NULL)
  {}

  int configure(const gc_string& cfg);
  int init();
  int try_linux_universal();
  int setup_tun();

  //void enableRead();
  //void onRead(const boost::system::error_code& error,
  //                    std::size_t /*bytes_transferred*/);


private:
  enum Type { UNDEFINED, LINUX_UNIVERSAL, LINUX_ETHERTAP, BSD_TUN, OSX_TUN, TAP0 };
  int _fd_raw;
  //int _mtu_in;
  //int _mtu_out;
  gc_string _dev_name;
  Type _type;
  //gc_string _near;
  //gc_string _mask;
  gc_string _if_cfg;
public:
  boost::asio::posix::stream_descriptor _fd;
  gc_streambuf* _buf;

  
};

extern void demux(Event e);

template <class T>
class Tun {
public:
  Tun(const char* cfg): _cfg(cfg), _device(), _buf(NULL) {}
  int init(){
    _device.configure(_cfg);
    return _device.init();    
  }
  void enableRead(){
    //if (_buf) //a read is already pending
    //  return;
    assert(!_buf);
    _buf = new (UseGC) gc_streambuf(1600, true);
    _device._fd.async_read_some(_buf->prepare(1600),
                                boost::bind(&Tun::onRead, this,
                                            boost::asio::placeholders::error,
                                            boost::asio::placeholders::bytes_transferred));
  }
  void onRead(const boost::system::error_code& error,
              std::size_t len){
    if (!error || error == boost::asio::error::message_size) {
      _buf->commit(len);
      Tuple *t = new T( Packet(_buf));
      _buf = NULL;
      Event ev(Event::RECV, t);
      demux(ev);
      enableRead();
    }else{
      std::cerr << "error reading from socket" << std::endl;
    }
  }
  template <class Buffer>
  void write_some(const Buffer& buf){
    _device._fd.write_some(buf);
  }
  const char* _cfg;
private:
  TunDevice _device;
  gc_streambuf* _buf;
};
#endif
