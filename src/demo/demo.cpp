/* $Id: demo.cpp 210 2009-04-14 22:14:22Z maoy $ */

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
#include "address.hpp"
#include "refTable.hpp"

using std::ostringstream;
using std::istringstream;


enum {
  ID_Tuple_Ping = TUPLE_RESERVED+1,
  ID_Tuple_Pong,
  ID_Tuple_test
};
typedef tuple<UDPAddress,UDPAddress,int> type_tuple_Ping;
DEFINE_TUPLE(Ping)
typedef tuple<UDPAddress,UDPAddress,int> type_tuple_Pong;
DEFINE_TUPLE(Pong)

/*
struct Tuple_test : public Tuple{
  int field0;
  long field1;
  std::string toString() const {
    return "";
  }
};
namespace boost{
  template<int N,class T>
  T& get(Tuple_test& t);
  template<>
  int& get<0>(Tuple_test & t){
    return t.field0;
  }
  template<>
  long& get<0>( Tuple_test & t){
    return t.field1;
  }
}
Tuple_test test;
typedef RefTable< Tuple_test, Keys<0> > Table_test;
Table_test t_test;
void testtable(){
  t_test.insert( new Tuple_test(0,1));
}
*/
void registerAllTuples(){
  LoadPointerHelper<NetIArchive,Tuple>::getInstance()
    ->registerClass<Tuple_Ping>();
  LoadPointerHelper<NetIArchive,Tuple>::getInstance()
    ->registerClass<Tuple_Pong>();
}

//tuple_map[ID_Tuple_Pong] = boost::bind(load_pointer<Tuple_Pong,NetIArchive>)
UdpSocket* g_udp;
std::deque<Event> taskQ;

void demux(Event e);

UDPAddress myAddr;

int parseAddr(const string& addr, string& ip, uint16_t& port);

template<class T>
void
send_action(T* stuff, const UDPAddress& locSpec){
  //std::string locSpec(locSpec1.c_str());
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
      gc_streambuf buf;
      //Tuple::toBuffer(stuff, buf);
      //Tuple::fromBuffer(buf);
      //std::cout << *t << std::endl;
      
    }
    taskQ.push_front(Event(Event::RECV, stuff));
    return;
  }
  //size_t len = Tuple::toBuffer(stuff, buf,65536);
  //std::cout << "send_action: len=" << len << '\n';
  //g_udp->sendTo(buf, len, locSpec);

  g_udp->sendTo(stuff, locSpec);
}

void pong_handler(Tuple* tt);
void pong_handler_2(Tuple* tt);

void
ping_handler(Tuple_Ping* tt){
  //assert(tt->id==ID_Tuple_Ping);
  Tuple_Ping *t = (Tuple_Ping*)tt;
  //switch source and dest
  //std::cout << "ping\n";
  Tuple_Pong* res = new Tuple_Pong(t->get<1>(), t->get<0>(), t->get<2>()+1);
  __typeof__(t->get<1>()) const& x = t->get<1>();
  {
    char x = ' ';
    x=x;
  }
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
  if (tt->get<2>() == 600000){ 
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
      pong_handler_2((Tuple_Pong*)e.t);
      break;
    default:
      break;
    }
    //boost::apply_visitor(demux_visitor(), *(e.t));

    if (taskQ.empty()) break;
    e = taskQ.back();
    taskQ.pop_back();
  }
}

void 
usage(){
  std::cout << "usage: mosaic address port\n";
  return;
}

struct add{};
struct sub{};

template <class T, class O1, class O2>
class Bin{
public:
  O1 o1;
  O2 o2;
  Bin(const O1 & oo1, const O2 & oo2):o1(oo1),o2(oo2){}
};

struct Add;
struct Sub;

typedef boost::variant<
  int,
  Add*
  > expr;

#include "lib/csv.hpp"
#include "boost/lexical_cast.hpp"
#include <fstream>
void csvtest(){
  int x;
  x = from_string_cast<int32_t>("12");
  std::cout << x << std::endl;
  from_string_cast<UDPAddress>("12.3.4.5:1234");
  CSVLoader l;
  if (l.open("test.csv")) {
    while (!l.eof()) l.next();
    l.close();
  }
  gc_string s = "12.3";
  boost::lexical_cast<double>(s);
}

//const gc_string const1="127.0.0.1:1234";
//const gc_string const2="127.0.0.1:1234";
const UDPAddress const1("127.0.0.1", 1234);
const UDPAddress const2("127.0.0.1", 1234);
int main (int argc, char **argv)
{
  
  csvtest();
  if (argc<3) {
    usage();
    return 1;
  }
  registerAllTuples();
  //myAddr = gc_string(argv[1])+":"+gc_string(argv[2]);
  myAddr = UDPAddress(argv[1], atoi(argv[2]));

  UdpSocket u("", atoi(argv[2]));
  u.init();
  g_udp = &u;
  Tuple* p = new (UseGC) Tuple_Ping(const1, const2, 1);

  Event e = Event(Event::RECV, p);
  callLater(0, boost::bind(demux, e));
  u.enableRead();
  iosv.run();
  
  return (0);
}

