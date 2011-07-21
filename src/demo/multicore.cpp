/* $Id: multicore.cpp 244 2009-06-30 19:27:03Z maoy $ */

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

//using namespace boost;
#include "event.hpp"
//#include "table.hpp"

//#include <event.h>
#include <gc/gc_cpp.h>
#include "udp.hpp"
#include "runtime.hpp"
#include "types.hpp"

#include "queue.hpp"

enum {
  ID_Tuple_Ping = TUPLE_RESERVED+1,
  ID_Tuple_Pong
};

typedef tuple<gc_string,gc_string,int> type_tuple_Ping;
DEFINE_TUPLE(Ping)
typedef tuple<gc_string,gc_string,int> type_tuple_Pong;
DEFINE_TUPLE(Pong)

#if 0
BEGIN_FROM_BUFFER_IMPL(NetIArchive)
INSTANCE_FROM_BUFFER_IMPL(Ping)
INSTANCE_FROM_BUFFER_IMPL(Pong)
END_FROM_BUFFER_IMPL()
#endif
void registerAllTuples(){
  REGISTER_TUPLE_ARCHIVE(Ping,NetIArchive,NetOArchive)
  REGISTER_TUPLE_ARCHIVE(Pong,NetIArchive,NetOArchive)
}

UdpSocket* g_udp;
std::deque<Event> taskQ;

typedef boost::function<void()> job_type;
Queue<job_type> jobQueue;

void demux(Event e);

gc_string myAddr;

int parseAddr(const string& addr, string& ip, uint16_t& port);

template<class T>
void
send_action(T* stuff, const gc_string& locSpec){
  //char buf[65536];
  if (locSpec == myAddr){
    if (1){/*
      using namespace boost::asio;
      using ip::udp;
      string ip; uint16_t port;
      parseAddr(locSpec, ip, port);
      udp::resolver resolver(iosv);
      udp::resolver::query query(udp::v4(), ip, port);
      udp::endpoint remote_endpoint = *resolver.resolve(query);*/
      //char buf[65536];
      //size_t l = Tuple::toBuffer(stuff, buf, 65536);
      //Tuple* t = Tuple::fromBuffer(buf, l);
      //t = new Tuple_Ping();
      //GC_gcollect();
      //delete t;
      //gc_streambuf buf;
      //Tuple::toBuffer(stuff, buf);
      //Tuple::fromBuffer(buf);
      //std::cout << *t << std::endl;
      
    }
    taskQ.push_front(Event(Event::RECV, stuff));
    return;
  }

  g_udp->sendTo(stuff, locSpec);
}

//void pong_handler(Tuple* tt);
//void pong_handler_2(Tuple* tt);

void
ping_handler(Tuple_Ping* tt){
  //assert(tt->id==ID_Tuple_Ping);
  Tuple_Ping *t = (Tuple_Ping*)tt;
  //switch source and dest
  //std::cout << "ping\n";
  Tuple_Pong* res = new Tuple_Pong(t->get<1>(), t->get<0>(), t->get<2>()+1);
  //res->source_ = t->dest_;
  //res->dest_ = t->source_;
  //res->t0_ =  t->t0_;
  send_action(res, res->get<0>());
}
void 
pong_handler(Tuple_Pong* tt){
  //assert(tt->id==ID_Tuple_Pong);
  Tuple_Pong *t = (Tuple_Pong*)tt;
  //std::cout << "pong\n";
  Tuple_Ping* res = new Tuple_Ping(t->get<1>(),t->get<0>(),t->get<2>());
  //if (res->t0_ % 10000== 0) std::cout << res->t0_<<"\n";
  //std::cout << res << std::endl;
  send_action(res, res->get<0>());
}

void
pong_handler_2(Tuple_Pong* t){
  Tuple_Pong* tt = (Tuple_Pong*)t;
  //  if (tt->get<2>() % 10000 == 0){
    //GC_gcollect();
    //std::cout << tt->toString() << std::endl;
    //std::cout << *t << std::endl;
  if (tt->get<2>() == 6000000){ 
    std::cout << *t << std::endl;
    exit(0);
      };
    //}
}
void
demux(Event e)
{
  //demux
  //std::cout << "in demux\n" << " " << (*e.t) << "\n";
  while (1){
    switch (e.t->type_id){
    case ID_Tuple_Ping:
      ping_handler((Tuple_Ping*)e.t);
      break;
    case ID_Tuple_Pong:
      pong_handler((Tuple_Pong*)e.t);
      //pong_handler_2((Tuple_Pong*)e.t);
      {
	jobQueue.enqueue( boost::bind(pong_handler_2, (Tuple_Pong*)(e.t)));
	//th.join();
      }
      break;
    default:
      break;
    }
    //boost::apply_visitor(demux_visitor(), *(e.t));

    if (taskQ.empty()) break;
    e = taskQ.back();
    taskQ.pop_back();
  }
  g_udp->enableRead();
}

void worker(){
  //std::cout << "in worker\n";
  job_type f;
  while(1){
    jobQueue.dequeue(f);
    //std::cout << "got a job\n";
    f();
  }
}

#include <boost/asio.hpp>
void thread_test(){
  using namespace boost::asio;
  boost::asio::posix::stream_descriptor fd(iosv);
  const char buf[] = "asdf";
  fd.assign(0);
  fd.write_some(buffer(buf));
  std::cout << fd.is_open() << std::endl;
}


void 
usage(){
  std::cout << "usage: mosaic address port\n";
  return;
}

const gc_string const1="127.0.0.1:1234";
const gc_string const2="127.0.0.1:1234";
int main (int argc, char **argv)
{
  //thread_test();
  //return 0;
  if (argc<3) {
    usage();
    return 1;
  }
  registerAllTuples();
  myAddr = gc_string(argv[1])+":"+gc_string(argv[2]);
  UdpSocket u(myAddr, atoi(argv[2]));
  u.init();
  g_udp = &u;
  Tuple* p = new (UseGC) Tuple_Ping(const1, const2, 1);
  Event e = Event(Event::RECV, p);
  callLater(0, boost::bind(demux, e));

  boost::thread 
    th( worker);

  /*boost::queue<int> q;
  q.enqueue(1);
  std::cout << "before dequeue" << q.size() << std::endl;
  int y;
  q.dequeue(y);
  std::cout << "after dequeue\n";
  */
  iosv.run();
  
  return (0);
}

