/* $Id: rawtun.cpp 210 2009-04-14 22:14:22Z maoy $ */

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

using namespace boost::asio;

UdpSocket* g_udp;
TunDevice* g_tun;

gc_string myAddr;

void 
usage(){
  std::cout << "usage: rawtun myaddress port tun_cfg peerip\n";
  return;
}

void demux(Event e){
}

const char* g_peerip;
ip::udp::endpoint g_remote;

void tun_on_read(const boost::system::error_code& error,
                 std::size_t len);
void read_from_tun();
void udp_on_read(const boost::system::error_code& error,
                 std::size_t len);
void read_from_udp();

//#define USE_BUF
gc_streambuf* g_tun_buf, *g_udp_buf;
char g_tun_sbuf[5000], g_udp_sbuf[5000];
using boost::asio::buffer;
inline void read_from_udp(){
  boost::asio::ip::udp::endpoint remote_endpoint;
#ifdef USE_BUF
  g_udp_buf = new (UseGC) gc_streambuf();
#endif
  g_udp->_skt->async_receive_from(
#ifdef USE_BUF
                                  g_udp_buf->prepare(1500),
#else
                                  buffer(g_udp_sbuf,5000),
#endif
                           remote_endpoint,
                           boost::bind(udp_on_read,
                                       boost::asio::placeholders::error,
                                       boost::asio::placeholders::bytes_transferred));
}
void udp_on_read(const boost::system::error_code& error,
                 std::size_t len){
  //std::cout << "udp on read: len = " << len <<"errcode=" << error << std::endl;
#ifdef USE_BUF
  g_udp_buf->commit(len);
  g_tun->_fd.write_some( g_udp_buf->data() );
  //g_udp_buf.consume( g_udp_buf.size() );
#else
  g_tun->_fd.write_some( buffer(g_udp_sbuf,len) );
#endif
  read_from_udp();
}
inline void read_from_tun(){
#ifdef USE_BUF
  g_tun_buf = new (UseGC) gc_streambuf();
#endif

  g_tun->_fd.async_read_some(
#ifdef USE_BUF
                             g_tun_buf->prepare(1500),
#else
                             buffer(g_tun_sbuf,5000),
#endif
                          boost::bind(tun_on_read,
                                      boost::asio::placeholders::error,
                                      boost::asio::placeholders::bytes_transferred));
}
void tun_on_read(const boost::system::error_code& error,
                    std::size_t len)
{
  using namespace boost::asio;

  //std::cout << "tun on read: len = " << len << std::endl;

#ifdef USE_BUF
  g_tun_buf->commit(len);
  g_udp->_skt->send_to(g_tun_buf->data(), g_remote);
  //g_tun_buf.consume( g_tun_buf.size() );
#else
  g_udp->_skt->send_to(buffer(g_tun_sbuf,len), g_remote);
#endif
  read_from_tun();
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
  gc_string tun_cfg = argv[3];
  g_peerip = argv[4];
  g_remote = ip::udp::endpoint(ip::address::from_string(g_peerip), atoi(argv[2]));

  UdpSocket u("", atoi(argv[2]));
  u.init();
  g_udp = &u;

  TunDevice tun;
  tun.configure(tun_cfg);
  tun.init();
  g_tun = &tun;
  
  read_from_tun();
  read_from_udp();

  //#define MULTI_THREAD
#ifndef MULTI_THREAD
  iosv.run();
#else
  boost::thread_group threads;
  for (int i=0;i<2;i++){
    threads.create_thread(boost::bind(&boost::asio::io_service::run, &iosv));
  }
  threads.join_all();
#endif
  return (0);
}

