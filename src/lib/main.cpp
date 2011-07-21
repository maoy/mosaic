/* $Id: main.cpp 218 2009-04-15 15:47:28Z maoy $ */

#include <unistd.h> //getopt

#include <string>

#include <iostream>

#include <boost/bind.hpp>
#include "boost/lexical_cast.hpp"

#include <gc/gc_cpp.h>
#include <gc/gc_allocator.h>
#include "lib/types.hpp"
#include "lib/refTable.hpp"
#include "lib/softTable.hpp"
#include "lib/time.hpp"

#include "lib/event.hpp"

#include "lib/udp.hpp"
#include "lib/runtime.hpp"
#include "lib/types.hpp"
#include "lib/address.hpp"
#include "lib/vector.hpp"
#include "lib/aggregate.hpp"
#include "lib/packet.hpp"
#include "lib/csv.hpp"

UdpSocket* g_udp;
EventQueue taskQ;

UDPAddress myAddr;

void local_send_action(Tuple* t){
  taskQ.enqueue(Event(Event::RECV, t));
}

template<class T>
void
send_action(T* stuff, const UDPAddress& locSpec){
  if (locSpec == myAddr){
    local_send_action(stuff);
    return;
  }
  g_udp->sendTo(stuff, locSpec);
}

void demux(Event e);

#include "gen.hpp"

void
demux(Event e)
{
  while (1){
    //if (e.t) std::cout << "in demux " << e.type << " "<< *e.t << std::endl;
    demux_handler(e);
    if (taskQ.empty()) {
      //reach the fixpoint, commit all
      taskQ.commitTables();
    }
    if (taskQ.empty()) break;
    e = taskQ.dequeue();
  }
}

void 
usage(const char* cmd){
  std::cout << "usage: " << cmd << " [-f facts.csv] address port\n";
  return;
}

int main (int argc, char * const argv[])
{
  const char* csv_filename = NULL;
  const char* addr = NULL;
  const char* port = NULL;
  //parsing command line args
  int opt;
  while ((opt = getopt(argc, argv, "f:")) != -1) {
    switch ( opt ) {
    case 'f':
      csv_filename = optarg;
      break;
    default:
      usage(argv[0]);
      return 1;
    }
  }

  if (optind >=(argc-1)) {
    usage(argv[0]);
    return 1;
  }
  addr = argv[optind];
  port = argv[optind+1];
  //end of parsing

  registerAllTuples();
  setup_signal_handler();

  myAddr = UDPAddress(addr, atoi(port) );
  UdpSocket u("", atoi(port) );
  u.init();
  g_udp = &u;

  facts();
  if (csv_filename)
    csv_facts(csv_filename);

  Event e = Event(Event::NONE, NULL);
  callLater(0, boost::bind(demux, e));
  u.enableRead();
  register_periodics();
  iosv.run();
  
  return (0);
}

