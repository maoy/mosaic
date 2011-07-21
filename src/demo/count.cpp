/* $Id: count.cpp 210 2009-04-14 22:14:22Z maoy $ */

#include <string>

#include <iostream>

#include <boost/bind.hpp>
#include <gc/gc_cpp.h>
#include <gc/gc_allocator.h>
#include "types.hpp"
#include "refTable.hpp"
#include "time.hpp"

#include "event.hpp"

#include "udp.hpp"
#include "runtime.hpp"
#include "types.hpp"
#include "address.hpp"

using std::ostringstream;

UdpSocket* g_udp;
EventQueue taskQ;

enum {
  ID_Tuple_counter = TUPLE_RESERVED+1,
  ID_Tuple_stat,
  ID_Tuple_limits
};

typedef tuple<int,int64_t> type_tuple_counter;
DEFINE_TUPLE(counter)

typedef RefTable<Tuple_counter, Keys<0> > Table_counter;
Table_counter t_counter(&taskQ);

typedef tuple<int,int64_t> type_tuple_limits;
DEFINE_TUPLE(limits)
typedef RefTable<Tuple_limits, Keys<0,1> > Table_limits;
Table_limits t_limits(&taskQ);

typedef tuple<gc_string,int,PosixTime> type_tuple_stat;
DEFINE_TUPLE(stat)
typedef RefTable<Tuple_stat, Keys<0,1,2> > Table_stat;
Table_stat  t_stat(&taskQ);

void registerAllTuples(){
  /*LoadPointerHelper<NetIArchive,Tuple>::getInstance()
    ->registerClass<Tuple_Ping>();
  LoadPointerHelper<NetIArchive,Tuple>::getInstance()
    ->registerClass<Tuple_Pong>();
  */
}

const gc_string g_const1 = "start";
const gc_string g_const2 = "stop";
void demux(Event e);

UDPAddress myAddr;

int parseAddr(const string& addr, string& ip, uint16_t& port);

void send_local_action(Tuple* t){
  taskQ.enqueue(Event(Event::RECV, t));
}

template<class T>
void
send_action(T* stuff, const UDPAddress& locSpec){
  if (locSpec == myAddr){
    send_local_action(stuff);
    return;
  }
  g_udp->sendTo(stuff, locSpec);
}

//stat("start", N+1, now() ) :- stat("stop", N, A);
void
ins_stat_handler(Tuple_stat* t0){
  if ( t0->get<0>() == g_const2 ) { //stop
    t_stat.insert( new Tuple_stat( g_const1, t0->get<1>()+1, PosixTime::now() ) );
  }
}

//stat("stop",   N, now() ) :- counter(N,MAX), limits(N,MAX);
void
ins_counter_handler(Tuple_counter* t0){
  boost::tuple<int,int64_t> t1key(t0->get<0>(),t0->get<1>());
  TABLE_PRIMARY_LOOKUP(limits, 1, &t1key ){
    std::cout << "stopped " << *t0 << std::endl;
    t_stat.insert( new Tuple_stat(g_const2, t0->get<0>(), PosixTime::now() ) );
  }
}

//counter(N,0L)   :- stat("start", N, A), limits(N,MAX);
void
ins_stat_handler2(Tuple_stat* t0){
  if ( t0->get<0>() == g_const1 ) { //start
    TABLE_FOREACH(limits, 1){
      if (t0->get<1>() == t1->get<0>())
	t_counter.insert( new Tuple_counter( t0->get<1>(), 0L ) );
    }  
  }
}


//counter(N,1L+I) :- counter#insert(N,I), notin limits(N,I);
void
ins_counter_handler2(Tuple_counter* t0){
  boost::tuple<int,int64_t> t1key(t0->get<0>(),t0->get<1>());
  TABLE_PRIMARY_LOOKUP(limits, 1, &t1key){
  }else{
    t_counter.insert( new Tuple_counter( t0->get<0>(), 1L+t0->get<1>() ) );
  }
}

//print(N, "counted to ", max<C>, STOP-START) :- stat("start", N, START) , stat("stop", N, STOP), counter(N,C);
void
ins_counter_handler3(Tuple_counter* t0){
  //std::cout << "counter_handler3" << *t0 << std::endl;
  TABLE_FOREACH(stat, 1){
    if (t0->get<0>() == t1->get<1>() && t1->get<0>()==g_const2){
      TABLE_FOREACH(stat, 2){
	if (t0->get<0>() == t2->get<1>() && t2->get<0>()==g_const1){
	  std::cout << "counted to " << t0->get<0>() << t2->get<2>() - t1->get<2>() << std::endl;
	}
      }
    }
  }
}
void
ins_stat_handler3(Tuple_stat* t0){
  if ( t0->get<0>() == g_const2 ) { //stop
    TABLE_FOREACH(stat, 1){
      if (t0->get<1>() == t1->get<1>() && t1->get<0>()==g_const1){ //start
	std::cout << "counted to " << t0->get<1>() << " took "<< t0->get<2>() - t1->get<2>() << std::endl;
      }
    }
  }
}

void demux_handler(Event e){
  if (e.type == Event::INSERTED) {
    switch (e.t->type_id){
    case ID_Tuple_counter:
      ins_counter_handler((Tuple_counter*)e.t);
      ins_counter_handler2((Tuple_counter*)e.t);
      ins_counter_handler3((Tuple_counter*)e.t);
      break;
    case ID_Tuple_stat:
      ins_stat_handler((Tuple_stat*)e.t);
      ins_stat_handler2((Tuple_stat*)e.t);
      ins_stat_handler3((Tuple_stat*)e.t);
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
  //if (e.t) std::cout << "in demux\n" << " " << (*e.t) << "\n";
  while (1){
    demux_handler(e);
    //std::cout << "taskq size:" << taskQ.size() << std::endl;
    if (taskQ.empty()) {
      //reach the fixture point, commit all
      /*
      t_stat.commit();
      t_counter.commit();
      t_limits.commit();
      */
      taskQ.commitTables();
      
    }
    if (taskQ.empty()) break;
    e = taskQ.dequeue();
  }
}

void 
usage(){
  std::cout << "usage: mosaic address port\n";
  return;
}

void
facts(){
  //stat("start",   1, java.lang.System.currentTimeMillis() );
  t_stat.insert( new Tuple_stat(g_const1, 1, PosixTime::now() ) );
  t_limits.insert( new Tuple_limits(1, 100L) );
  t_limits.insert( new Tuple_limits(2, 1000L) );
  t_limits.insert( new Tuple_limits(3, 5000L) );
  t_limits.insert( new Tuple_limits(4, 10000L) );
  t_limits.insert( new Tuple_limits(5, 50000L) );
  t_limits.insert( new Tuple_limits(6, 100000L) );
  t_limits.insert( new Tuple_limits(7, 500000L) );
  
}
int main (int argc, char **argv)
{
  if (argc<3) {
    usage();
    return 1;
  }
  registerAllTuples();
  myAddr = UDPAddress(argv[1], atoi(argv[2]));

  UdpSocket u("", atoi(argv[2]));
  u.init();
  g_udp = &u;

  facts();

  Event e = Event(Event::NONE, NULL);
  callLater(0, boost::bind(demux, e));
  u.enableRead();
  iosv.run();
  
  return (0);
}

