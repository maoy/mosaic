/* $Id: tunTest.cpp 210 2009-04-14 22:14:22Z maoy $ */

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
#include "address.hpp"

enum {
  ID_Tuple_Ping = TUPLE_RESERVED+1,
  ID_Tuple_tun,
  ID_Tuple_instun,
  ID_Tuple_nat,
};

typedef tuple<Packet> type_tuple_instun;
DEFINE_TUPLE(instun)
typedef tuple<Packet> type_tuple_tun;
DEFINE_TUPLE(tun)
typedef tuple<uint32_t, UDPAddress> type_tuple_nat;
DEFINE_TUPLE(nat)
/*
BEGIN_FROM_BUFFER_IMPL(NetIArchive)
INSTANCE_FROM_BUFFER_IMPL(nat)
INSTANCE_FROM_BUFFER_IMPL(instun)
END_FROM_BUFFER_IMPL()
*/
void registerAllTuples(){
  REGISTER_TUPLE_ARCHIVE(instun,NetIArchive,NetOArchive)
}

UdpSocket* g_udp;
Tun<Tuple_tun>* g_tun;
EventQueue taskQ;

typedef RefTable<Tuple_nat,
                 Keys<0>
                 > Table_nat;
Table_nat t_nat(&taskQ);

UDPAddress myAddr;

template<class T>
void
send_action(T* stuff, const UDPAddress& locSpec){
  if (locSpec == myAddr){
    /*
    if (stuff->id == ID_Tuple_tun){
      Tuple_tun* t = static_cast<Tuple_tun*>(stuff);
      g_tun->write_some( t->get<0>()._buf->data() );
    }
    else*/
    taskQ.enqueue(Event(Event::RECV, stuff));  
    return;
  }

  g_udp->sendTo(stuff, locSpec);
}

void tun_handler(Tuple_tun* t){
  uint32_t dst = t->get<0>().dstIP();
  //std::cout << dst << std::endl;
  TABLE_PRIMARY_LOOKUP(nat,1, &dst){
    //std::cout << "sending......." << t1->get<1>() << std::endl;
    Tuple_instun* res = new Tuple_instun( t->get<0>() );
    send_action( res, t1->get<1>());
  }
}

void ins_tun_handler(Tuple_instun* t){
  g_tun->write_some( t->get<0>()._buf->data() );  
}

void demux_handler(Event e){
  if (e.type==Event::RECV){
    switch (e.t->type_id){
    case ID_Tuple_nat:
      break;
    case ID_Tuple_tun:
      tun_handler(static_cast<Tuple_tun*>(e.t) );
      break;
    case ID_Tuple_instun:
      ins_tun_handler(static_cast<Tuple_instun*>(e.t));
      break;
    default:
      break;
    }
  }
}

void
demux(Event e)
{
  //demux
  //if (e.type!=Event::NONE)
  //  std::cout << "in demux\n" << " " << (*e.t) << "\n";
  while (1){
    demux_handler(e);
    //std::cout << "taskq size:" << taskQ.size() << std::endl;
    if (taskQ.empty()) {
      //reach the fixture point, commit all
      t_nat.commit();
    }
    if (taskQ.empty()) break;
    e = taskQ.dequeue();
  }
}

void 
usage(){
  std::cout << "usage: tunTest address port tun_cfg\n";
  return;
}

int main (int argc, char **argv)
{
  //tun_test();
  //return 0;
  if (argc<4) {
    usage();
    return 1;
  }
  registerAllTuples();
  UDPAddress udpaddr("1.0.0.19", 25);
  std::cout << udpaddr << std::endl;
  setup_signal_handler();
  //myAddr = gc_string(argv[1])+":"+gc_string(argv[2]);
  myAddr = UDPAddress(argv[1], atoi(argv[2]) );
  gc_string tun_cfg = argv[3];
  UdpSocket u("", atoi(argv[2]));
  u.init();
  g_udp = &u;

  Tun<Tuple_tun> tun(tun_cfg.c_str());
  tun.init();
  g_tun = &tun;
  tun.enableRead();
  u.enableRead();

  //facts
  struct in_addr addr;
  inet_aton("1.0.0.11", &addr);
  t_nat.insert( new Tuple_nat(addr.s_addr, UDPAddress("192.168.1.43",1234)) );
  inet_aton("1.0.0.10", &addr);
  t_nat.insert( new Tuple_nat(addr.s_addr, UDPAddress("192.168.1.42",1234)) );
  t_nat.commit();
  demux(Event(Event::NONE, NULL));  
  iosv.run();
  return (0);
}

