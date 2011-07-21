/* $Id: perf10.cpp 210 2009-04-14 22:14:22Z maoy $ */

#include <sstream>
#include <iostream>
#include <list>

//#include <unistd.h> //usleep

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
//#include <boost/tuple/tuple.hpp>
//#include "boost/tuple/tuple_comparison.hpp"
//#include "boost/tuple/tuple_io.hpp"
//#include <typeinfo>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/bind.hpp>

#include <boost/fusion/container/vector.hpp>

using namespace boost;
#include "tuple.hpp"
#include "refTable.hpp"
#include "runtime.hpp"
#include <gc/gc_cpp.h>

using std::ostringstream;
using std::istringstream;

/////////////Tuple definitions
enum {
  ID_Tuple_num = TUPLE_RESERVED+1,
  ID_Tuple_periodic,
  ID_Tuple_beginTime
};

typedef tuple<gc_string,int,int> type_tuple_periodic;
DEFINE_TUPLE(periodic)
typedef tuple<gc_string,int,int> type_tuple_num;
DEFINE_TUPLE(num)
typedef tuple<gc_string,int,int> type_tuple_beginTime;
DEFINE_TUPLE(beginTime)

/*
BEGIN_FROM_BUFFER_IMPL(NetIArchive)
INSTANCE_FROM_BUFFER_IMPL(num)
INSTANCE_FROM_BUFFER_IMPL(beginTime)
END_FROM_BUFFER_IMPL()
*/
//////////////// Table definitions
typedef RefTable<Tuple_beginTime, 
	      Keys<0,1> >
Table_beginTime;
/////////////////
EventQueue taskQ;
Table_beginTime t_beginTime(&taskQ);

gc_string myAddr;

void 
send_local_action(Tuple* stuff){
    taskQ.enqueue(Event(Event::RECV, stuff));  
}
void
send_action(Tuple* stuff, const gc_string& locSpec){
  //char buf[65536];
  if (locSpec == myAddr){
    return send_local_action(stuff);
  }
  //FIXME
  size_t len = 0;//stuff->toBuffer(buf);
  std::cout << "send_action: len=" << len << '\n';
  //udp->sendTo(buf, len, locSpec);
}

//n1 num(@NI, X) :- periodic(@NI, E, 0, 1), X:=0.
void
n1_handler(Tuple_periodic* t){
  int X = 0;
  Tuple_num* res = new Tuple_num(t->get<0>(), X, 0);
  send_action(res, res->get<0>());
}

//n2 num(@NI, X) :- num(@NI, Y), beginTime(@NI, T), beginTime(@NI, T1),beginTime(@NI, T), beginTime(@NI, T1),beginTime(@NI, T), beginTime(@NI, T1),beginTime(@NI,
//T), beginTime(@NI, T1),    beginTime(@NI, T), beginTime(@NI, T1), X:= Y+1.

void 
n2_handler(Tuple_num* t){
  //std::cout << *t << std::endl;
  TupleList res;
  if (t->get<1>() == 10000000) 
    {std::cout << t << std::endl;exit(0);}

  TABLE_FOREACH(beginTime,1){
    TABLE_FOREACH(beginTime,2){
      TABLE_FOREACH(beginTime,3){
	TABLE_FOREACH(beginTime,4){
	  TABLE_FOREACH(beginTime,5){
	    TABLE_FOREACH(beginTime,6){
	      TABLE_FOREACH(beginTime,7){
		TABLE_FOREACH(beginTime,8){
		  TABLE_FOREACH(beginTime,9){
		    TABLE_FOREACH(beginTime,10){
		      if (t->get<0>() == t1->get<0>())
			if (t->get<0>() == t2->get<0>())
			  if (t->get<0>() == t3->get<0>())
			    if (t->get<0>() == t4->get<0>())
			      if (t->get<0>() == t5->get<0>()){
				if (t->get<0>() == t6->get<0>()){
				  if (t->get<0>() == t7->get<0>()){
				    if (t->get<0>() == t8->get<0>()){
				      if (t->get<0>() == t9->get<0>()){
					if (t->get<0>() == t10->get<0>()){
					  int y = t->get<1>()+1;
					  Tuple_num* res = new Tuple_num(t->get<0>(), y, 0);
					  send_action(res, res->get<0>());
					}
				      }
				    }
				  }
				}
			      }
		    }
		  }
		}
	      }
	    }
	  }
	}
      }
    }
  }
}

void
demux(Event e)
{
  //demux
  std::cout << "in demux\n" << " " << (*e.t) << "\n";
  while (1){
    switch (e.t->type_id){
    case ID_Tuple_periodic:
      n1_handler((Tuple_periodic*)e.t);
      break;
    case ID_Tuple_num:
      n2_handler((Tuple_num*)e.t);
      break;
    case ID_Tuple_beginTime:
      break;
    default:
      break;
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

int
main (int argc, char **argv)
{
  if (argc<3) {
    usage();
    return 1;
  }
  myAddr = gc_string(argv[1])+":"+gc_string(argv[2]);
  //facts
  t_beginTime.insert( new Tuple_beginTime("127.0.0.1:1234", 0,0) );
  t_beginTime.commit();
  Tuple* p = new Tuple_periodic("127.0.0.1:1234",0, 1);
  Event e = Event(Event::RECV, p);
  callLater(0, boost::bind(demux, e));
  /*
  asio::deadline_timer t(iosv, boost::posix_time::seconds(0));
  t.async_wait(boost::bind(demux,e));
  */
  try{
    iosv.run();
  }
  catch (std::exception& e){
    std::cerr << e.what() << std::endl;
  }
  return (0);
}

