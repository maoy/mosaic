/* $Id: txn.cpp 225 2009-06-16 20:49:02Z maoy $ */

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
#include "lib/txnManager.hpp"

UdpSocket* g_udp;
TxnManager* g_txnManager = NULL;
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

#include "reachable_txn.hpp"

////////////////////////////
void view_maintenance(TxnContext& ctx)
{
  while (!ctx.deltaQ.empty()){
    do {
      Event e = ctx.deltaQ.dequeue();
      ctx.triggerQ.enqueue(e);
      //fire those delta rewrite rules
      view_handler(e, ctx);
    } while ( !ctx.deltaQ.empty() );
    // reach a temp fixpoint commit all
    ctx.deltaQ.commitTables(); // commit changes in views
    //view change may trigger new updates on other views
  }
}

void mainloop(Event e)
{
  TxnContext ctx = g_txnManager->create();
  //ctx should have a new event_queue and delta_queue initiated
  //associate every table with delta queue
  event_handler(e, ctx); //only handles user-events, i.e. non-db events
  ctx.deltaQ.commitTables();

  do {
    //now deltaQ may have some db events
    view_maintenance( ctx );
    //now delta is empty, trigger queue might be non-empty

    while (! ctx.triggerQ.empty() ){
      Event e = ctx.triggerQ.dequeue();
      trigger_handler(e, ctx);
    }
    ctx.deltaQ.commitTables();
  } while ( !ctx.deltaQ.empty() );

  g_txnManager->commit(ctx);
  //send all events in emitQ
  //FIXME: this is not used yet. events are emitted right away
  //so there is no way to rollback events sent
}

///////////////
void
demux(Event e)
{
  if (e.t) 
    std::cout << "in demux " << e.type << " "<< *e.t << std::endl;
  else
    std::cout << "in demux (null tuple) " << e.type <<  std::endl;
  assert(e.type==Event::RECV || e.type == Event::NONE);
  mainloop(e);
  while (!taskQ.empty()){
    e = taskQ.dequeue();
    std::cout << "in demux loop" << e.type << " "<< *e.t << std::endl;
    assert(e.type==Event::RECV || e.type == Event::NONE);
    mainloop(e);
  }
  
  /*
  TxnContext ctx = g_txnManager->create();
  demux_handler(e, ctx);
  taskQ.commitTables();
  view_maintenance( taskQ, ctx );
  g_txnManager->commit(ctx);
  */
}


void 
usage(const char* cmd){
  std::cout << "usage: " << cmd << " [-f facts.csv] address port\n";
  return;
}

TxnManager* initTxnManager() {
  //create a workspace
  WorkspacePtr wsp = new Workspace();
  //fill in those tables
  initWorkspace(wsp);
  return new TxnManager( wsp );
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

  g_txnManager = initTxnManager(); // new

  myAddr = UDPAddress(addr, atoi(port) );
  UdpSocket u("", atoi(port) );
  u.init();
  g_udp = &u;

  TxnContext ctx = g_txnManager->create();
  facts(ctx);
  if (csv_filename)
    csv_facts(csv_filename, ctx);
  //taskQ.commitTables();
  ctx.deltaQ.commitTables();
  view_maintenance( ctx );
  std::cout << "taskQ size=" << taskQ.size() << std::endl;
  g_txnManager->commit(ctx);
  Event e = Event(Event::NONE, NULL);
  callLater(0, boost::bind(demux, e));
  u.enableRead();
  register_periodics();
  iosv.run();
  
  return (0);
}

