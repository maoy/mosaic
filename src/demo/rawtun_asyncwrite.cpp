/* $Id: rawtun_asyncwrite.cpp 210 2009-04-14 22:14:22Z maoy $ */

#include <string>

#include <sstream>
#include <iostream>
#include <deque>

#include <unistd.h> //usleep

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/tuple/tuple.hpp>
#include "boost/tuple/tuple_comparison.hpp"
#include "boost/tuple/tuple_io.hpp"
#include <boost/thread/thread.hpp>
#include <boost/function.hpp>

#include <typeinfo>
#include <boost/variant.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/bind.hpp>
#include <gc/gc_allocator.h>

#include "event.hpp"
//#include "table.hpp"

//#include <event.h>
#include <gc/gc_cpp.h>
#include "udp.hpp"
#include "runtime.hpp"
#include "types.hpp"
#include "queue.hpp"
#include "refTable.hpp"

#include "packet.hpp"
#include <boost/asio.hpp>
#include "tun.hpp"

UdpSocket* g_udp;
TunDevice* g_tun;

gc_string myAddr;

void 
usage(){
  std::cout << "usage: tunTest address port tun_cfg\n";
  return;
}

void demux(Event e){
}

template<>
Tuple* fromArchiveImpl<NetIArchive>(unsigned short, NetIArchive&){
  return NULL;
}

const gc_string const1="127.0.0.1:1234";
const gc_string const2="127.0.0.1:1234";

const char* g_peerip;
void tun_on_read(const boost::system::error_code& error,
                 std::size_t len);
void read_from_tun();
void udp_on_read(const boost::system::error_code& error,
                 std::size_t len);
void read_from_udp();

gc_streambuf g_tun_buf,g_udp_buf;

void read_from_udp(){
  boost::asio::ip::udp::endpoint remote_endpoint;
  g_udp->_skt->async_receive_from(
                           g_udp_buf.prepare(1500),
                           remote_endpoint,
                           boost::bind(udp_on_read,
                                       boost::asio::placeholders::error,
                                       boost::asio::placeholders::bytes_transferred));
}

void tun_write_done(const boost::system::error_code& error, std::size_t len){
  g_udp_buf.consume( g_udp_buf.size() );
  read_from_udp();
}

void udp_on_read(const boost::system::error_code& error,
                 std::size_t len){
  //std::cout << "udp on read: len = " << len <<"errcode=" << error << std::endl;
  g_udp_buf.commit(len);
  g_tun->_fd.async_write_some( g_udp_buf.data(), tun_write_done );
}
void read_from_tun(){
  g_tun->_fd.async_read_some(g_tun_buf.prepare(1500),
                          boost::bind(tun_on_read,
                                      boost::asio::placeholders::error,
                                      boost::asio::placeholders::bytes_transferred));
}
void udp_send_done(const boost::system::error_code& error, std::size_t len){
  g_tun_buf.consume( g_tun_buf.size() );
  read_from_tun();//yes, not udp
}
void tun_on_read(const boost::system::error_code& error,
                    std::size_t len)
{
  //std::cout << "tun on read: len = " << len << std::endl;
  g_tun_buf.commit(len);
  using namespace boost::asio;
  ip::udp::endpoint remote_endpoint(ip::address::from_string(g_peerip), 1234);
  
  g_udp->_skt->async_send_to(g_tun_buf.data(), remote_endpoint, udp_send_done);
}

int main (int argc, char **argv)
{
  //tun_test();
  //return 0;
  if (argc<5) {
    usage();
    return 1;
  }
  setup_signal_handler();
  myAddr = gc_string(argv[1])+":"+gc_string(argv[2]);
  gc_string tun_cfg = argv[3];
  g_peerip = argv[4];
  UdpSocket u(myAddr, atoi(argv[2]));
  u.init();
  g_udp = &u;

  TunDevice tun;
  tun.configure(tun_cfg);
  tun.init();
  g_tun = &tun;
  
  read_from_tun();
  read_from_udp();

  iosv.run();
  return (0);
}

