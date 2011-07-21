/* $Id: udp.cpp 210 2009-04-14 22:14:22Z maoy $ */

#include <boost/bind.hpp>

#include "udp.hpp"
#include "tuple.hpp"
#include "event.hpp"
using namespace boost::asio;
using ip::udp;
using std::size_t;

int
UdpSocket::init(){
  //listen
  //udp::resolver resolver(*_io);
  //udp::resolver::query query(udp::v4(),_name, _port);
  _skt = new (UseGC) udp::socket(*_io, udp::endpoint(udp::v4(),_port) );
  assert(_skt);
  socket_base::broadcast option(true);
  _skt->set_option(option);
  return 0;
}

void UdpSocket::enableRead(){
  assert(!_buf); //not allow re-enter enableRead
  _buf = new (UseGC) gc_streambuf(1600, true);
  _skt->async_receive_from(
                           _buf->prepare(1600),
                           _remote_endpoint,
                           boost::bind(&UdpSocket::onRead, this,
                                       boost::asio::placeholders::error,
                                       boost::asio::placeholders::bytes_transferred));
  
}

extern void demux(Event e);

void UdpSocket::onRead(const boost::system::error_code& error,
                    std::size_t len)
{
  if (!error || error == boost::asio::error::message_size) {
    _buf->commit(len);
    //std::cout << "recv " << len << std::endl;
    NetIArchive ia(*_buf);
    //Tuple* t = Tuple::fromBuffer(ia);
    uint16_t mn;
    ia & mn;
    if (mn != magic_number){
      throw std::runtime_error("magic number mistmatch");
    }
    poly_ptr<Tuple> ptr;
    ia & ptr;
    Tuple* t = ptr.get_ptr();
    _buf = NULL;
    Event ev(Event::RECV, t);
    demux(ev);
    enableRead();
  }else{
    std::cerr << "error reading from a UDP socket" << std::endl;
  }
}
