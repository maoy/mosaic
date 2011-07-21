/* $Id: bestpath.cpp 210 2009-04-14 22:14:22Z maoy $ */

//-*-  indent-tabs-mode:nil; c-basic-offset:2  -*-
#include <sstream>
#include <iostream>
#include <list>

//#include <unistd.h> //usleep

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/bind.hpp>

#include "runtime.hpp"
#include "types.hpp"
#include "vector.hpp"
#include "time.hpp"

//#include <boost/fusion/container/vector.hpp>

//using namespace boost;
#include "tuple.hpp"
#include "refTable.hpp"

#include "aggregate.hpp"

#include <gc/gc_cpp.h>

//using std::ostringstream;
//using std::istringstream;

enum {
  ID_Tuple_reserved = TUPLE_RESERVED,
  ID_Tuple_periodic1,
  ID_Tuple_periodic2,
  ID_Tuple_path,
  ID_Tuple_bestPath,
  ID_Tuple_link,
  ID_Tuple_pathCount
};
typedef tuple<String,Integer> type_tuple_periodic1;
DEFINE_TUPLE(periodic1)
typedef tuple<std::string,int> type_tuple_periodic2;
DEFINE_TUPLE(periodic2)
typedef tuple<String,String,Integer,Vector<String> > type_tuple_path;
DEFINE_TUPLE(path)
typedef tuple<String,String,Integer, Vector<String> > type_tuple_bestPath;
DEFINE_TUPLE(bestPath)
typedef tuple<String,String,Integer > type_tuple_pathCount;
DEFINE_TUPLE(pathCount)
typedef tuple<  String, String, Integer > type_tuple_link;
DEFINE_TUPLE(link) //src,dest,cost

/*
BEGIN_FROM_BUFFER_IMPL(NetIArchive)
INSTANCE_FROM_BUFFER_IMPL(path)
INSTANCE_FROM_BUFFER_IMPL(bestPath)
INSTANCE_FROM_BUFFER_IMPL(link)
END_FROM_BUFFER_IMPL()
*/

typedef RefTable<Tuple_bestPath,
	      Keys<0,1>,
	      Keys<0> 
	      > Table_bestPath;
typedef RefTable<Tuple_path,
	      Keys<3>, //primary key
	      Keys<0,1> //group by key
	      > Table_path;
typedef RefTable<Tuple_link,
	      Keys<0,1>,
	      Keys<1>
	      > Table_link;
typedef RefTable<Tuple_pathCount,
                 Keys<0,1> > Table_pathCount;
/////////////////
EventQueue taskQ;
Table_path t_path(&taskQ);
Table_bestPath t_bestPath(&taskQ, false);
Table_link t_link(&taskQ);
Table_pathCount t_pathCount(&taskQ, false);
/////////////////
/*
void testAgg(){
  Table_path t_path(&taskQ);
  Table_bestPathCost t_cost(&taskQ);
  typedef Aggregate<Table_path, Keys<0>, 1> agg_type;
  Tuple_path* t;
  t = new Tuple_path("A", 2, "A->B");
  t_path.insert( t );
  t_path.commit();
  {
    agg_type agg(&t_path, t);
    t_cost.insert( new Tuple_bestPathCost(t->get<0>(), agg.min()) );
    t_cost.commit();
  }

  t = new Tuple_path("A", 1, "A->C");
  t_path.insert( t );
  t_path.commit();
  {
    agg_type agg(&t_path, t);
    t_cost.insert( new Tuple_bestPathCost(t->get<0>(), agg.min()) );
    t_cost.commit();
  }

  t = new Tuple_path("A", 0, "A->B");
  t_path.insert( t );
  t_path.commit();
  {
    agg_type agg(&t_path, t);
    t_cost.insert( new Tuple_bestPathCost(t->get<0>(), agg.min()) );
    t_cost.commit();
  }

  t = new Tuple_path("A", 0, "A->C");
  t_path.erase( t );
  t_path.commit();
  {
    agg_type agg(&t_path, t);
    t_cost.insert( new Tuple_bestPathCost(t->get<0>(), agg.min()) );
    t_cost.commit();
  }

  t_path.dump();
  t_cost.dump();
  std::cout << taskQ.size() << std::endl;
  BOOST_FOREACH(Event& e, taskQ){
    std::cout << e << std::endl;
  }
}
*/
void demux(Event e);

std::string myAddr;

void 
send_local_action(Tuple* stuff){
    taskQ.enqueue(Event(Event::RECV, stuff));  
}
void
send_action(Tuple* stuff, const std::string& locSpec){
  return send_local_action(stuff);
  if (locSpec == myAddr){
    return send_local_action(stuff);
  }
  //FIXME
  size_t len = 0;//stuff->toBuffer(buf);
  std::cout << "send_action: len=" << len << '\n';
  //udp->sendTo(buf, len, locSpec);
}

const std::string const1("first");
const std::string const2("second");

/* r1 tLink(@NI,C,P) :- periodic(@NI,0), C:=1, P:="first"*/
void r1_handler(Tuple_periodic1* t){
  Tuple_link* res = new Tuple_link(String("B"),String("D"), 1);
  t_link.insert(res);

  Event e2 = Event(Event::RECV, t);
  callLater(2,boost::bind(demux,e2));
}

/* r2 tPath(@NI,C,P) :- periodic(@NI,1), C:=2, P:="second"*/
void r2_handler(Tuple_periodic2* t){
  Tuple_link* res = new Tuple_link(String("B"),String("D"), 1);
  //t_link.insert(res);
  t_link.erase(res);

  Event e2 = Event(Event::RECV, t);
  callLater(2,boost::bind(demux,e2));

}

/*r3 path(@X,Y,C,P) :- link(@X,Y,C),P:=[X,Y]*/
void r3_ins_handler(Tuple_link* t){
  t_path.insert( new Tuple_path(
				t->get<0>(),
				t->get<1>(),
				t->get<2>(),
				f_push_back(f_make_vector(t->get<0>()),
					    t->get<1>()) ) );
}

void r3_del_handler(Tuple_link* t){
  t_path.erase( new Tuple_path(
				t->get<0>(),
				t->get<1>(),
				t->get<2>(),
				f_push_back(f_make_vector(t->get<0>()),
					    t->get<1>()) ) );
}

/* on insert tPath(_), tPath(X,Y,_,_), groupBy(X,Y) => pathCount(X,Y,count<*>);*/
void count_handler(Tuple_path* t){
  //AggTable<Table_path, Keys<0,1>,2> aggt(&t_path);
  //*(aggt.begin());
  typedef Keys<0,1> groupkey;
  TABLE_FOREACH_KEY( path,1, groupkey ) {
    Aggregate<Table_path, groupkey, 2> agg(&t_path, _key_1);
    std::cout << "counting.. "<< *_key_1 << agg.count() << std::endl;
  }
}
/*r4 tBestPath(X, Y , a_MIN<C>, P) :- tPath(X, Y, C, P).*/
void r4_insdel_handler(Tuple_path* t){
  Aggregate<Table_path, Keys<0,1>, 2> agg(&t_path, t);
  Tuple_path* t2 = agg.min_tuple();
  if (t2){
    t = t2;
    t_bestPath.insert(new Tuple_bestPath(t->get<0>(),
					 t->get<1>(),
					 t->get<2>(),
					 t->get<3>())
		      );
  }
  else
    t_bestPath.insert(new Tuple_bestPath(t->get<0>(),
					 t->get<1>(),
					 Integer(),
					 Vector<String>())
		      );
    
}


//r5 tPath(X,Y,C, P) :- tLink(X,Z,C),tBestPath(Z,Y,C,P), 
//C:=C1+C2,f_member(P,X), P:=X::P2.

void r5_ins1_handler(Tuple_link* t){
  //tuple<String> * key1 = new (UseGC) tuple<String>(t->get<1>() );
  //String * key1 = &(t->get<1>());
  String & key1(t->get<1>());
  TABLE_SECONDARY_LOOKUP(bestPath,1,&key1,Keys<0>)
  {
    //assert(t->get<1>() == t1->get<0>() );
    if (! f_is_member(t1->get<3>(), t->get<0>() )){
      t_path.insert(new Tuple_path(t->get<0>(),
				   t1->get<1>(),
				   t->get<2>()+t1->get<2>(),
				   f_make_vector(t->get<0>())+ t1->get<3>()
				   ) );
    }
  }
}
void r5_del1_handler(Tuple_link* t){
  //tuple<String> * key1 = new (UseGC) tuple<String>(t->get<1>() );
  String * key1 = &(t->get<1>());
  TABLE_SECONDARY_LOOKUP(bestPath,1,key1,Keys<0>)
  {
    if (! f_is_member(t1->get<3>(), t->get<0>() )){
      t_path.erase(new Tuple_path(t->get<0>(),
				  t1->get<1>(),
				  t->get<2>()+t1->get<2>(),
				  f_make_vector(t->get<0>())+ t1->get<3>()
				  ) );
    }
  }
}

void r5_ins2_handler(Tuple_bestPath* t){
  String * key1 = &(t->get<0>());
  //tuple<String> * key1 = new (UseGC) tuple<String>(t->get<0>() );
  TABLE_SECONDARY_LOOKUP(link,1,key1,Keys<1>){
    if (! f_is_member(t->get<3>(), t1->get<0>() )){
      t_path.insert(new Tuple_path(t1->get<0>(),
				   t->get<1>(),
				   t->get<2>()+t1->get<2>(),
                                   f_make_vector(t1->get<0>())+ t->get<3>()
				   ) );
    }
  }
}

void r5_del2_handler(Tuple_bestPath* t){
  String * key1 = &(t->get<0>());
  //tuple<String> * key1 = new (UseGC) tuple<String>(t->get<0>() );
  TABLE_SECONDARY_LOOKUP(link,1,key1,Keys<1>){
    if (! f_is_member(t->get<3>(), t1->get<0>() )){
      t_path.erase(new Tuple_path(t1->get<0>(),
                                  t->get<1>(),
                                  t->get<2>()+t1->get<2>(),
                                  f_make_vector(t1->get<0>())+ t->get<3>()
                                  ) );
    }
  }
}
void
demux(Event e)
{
  //demux
  std::cout << "in demux\n" << " " << e << std::endl;
  while (1){
    if (e.type == Event::INSERTED) {
      switch (e.t->type_id){
      case ID_Tuple_path:
	r4_insdel_handler( static_cast<Tuple_path*>(e.t) );
        count_handler( static_cast<Tuple_path*>(e.t) );
	break;
      case ID_Tuple_link:
	r3_ins_handler( static_cast<Tuple_link*>(e.t) );
	r5_ins1_handler( static_cast<Tuple_link*>(e.t) );	
	break;
      case ID_Tuple_bestPath:
	r5_ins2_handler( static_cast<Tuple_bestPath*>(e.t) );
	break;
      }
    }
    else if (e.type == Event::DELETED) {
      switch (e.t->type_id){
      case ID_Tuple_path:
	r4_insdel_handler( static_cast<Tuple_path*>(e.t) );
	break;
      case ID_Tuple_link:
	r3_del_handler( static_cast<Tuple_link*>(e.t) );
	r5_del1_handler( static_cast<Tuple_link*>(e.t) );	
	break;
      case ID_Tuple_bestPath:
	r5_del2_handler( static_cast<Tuple_bestPath*>(e.t) );
	break;
      }
    }
    else if (e.type == Event::RECV){
      switch (e.t->type_id){
      case ID_Tuple_periodic1:
	r1_handler( static_cast<Tuple_periodic1*>(e.t) );
	break;
      case ID_Tuple_periodic2:
	r2_handler( static_cast<Tuple_periodic2*>(e.t) );
	break;
      }
    }

    std::cout << "taskq size:" << taskQ.size() << std::endl;
    if (taskQ.empty()) {
      //reach the fixture point, commit all
      t_path.commit();
      //t_bestPathCost.commit();
      t_bestPath.commit();
      //t_bestPath2.commit();
      t_link.commit();
    }
    if (taskQ.empty()) break;
    e = taskQ.dequeue();
  }
  t_path.dump();
  //t_bestPathCost.dump();
  t_bestPath.dump();
  //t_bestPath2.dump();
  t_link.dump();
}

void 
usage(){
  std::cout << "usage: mosaic address port\n";
  return;
}

int
main (int argc, char **argv)
{
  //testAgg();
  //return 0;
  //std::cout << f_now() << std::endl;
  if (argc<3) {
    usage();
    return 1;
  }
  myAddr = std::string(argv[1])+":" + std::string(argv[2]);
  //periodics
  Tuple* p = new (UseGC) Tuple_periodic1(String("127.0.0.1:1234"),0);
  Event e = Event(Event::RECV, p);

  callLater(0, boost::bind(demux,e));

  Tuple* p2 = new (UseGC) Tuple_periodic2("127.0.0.1:1234",0);
  Event e2 = Event(Event::RECV, p2);
  callLater(1, boost::bind(demux,e2));
  

  //facts
  t_link.insert( new Tuple_link(String("A"), String("B"), 1) );
  t_link.insert( new Tuple_link(String("B"), String("C"), 1) );
  t_link.insert( new Tuple_link(String("A"), String("C"), 1) );
  t_link.insert( new Tuple_link(String("C"), String("D"), 1) );
  //t_link.insert( new Tuple_link(String("D"), String("E"), 1) );
  t_link.commit();

  iosv.run();
  
  return (0);
}

