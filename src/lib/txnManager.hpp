/* $Id: txnManager.hpp 225 2009-06-16 20:49:02Z maoy $ */

#ifndef __TXN_MANAGER_HPP__
#define __TXN_MANAGER_HPP__

#include "workspace.hpp"
#include "event.hpp"

typedef uint32_t TxnID;
struct TxnContext {
  enum State {
    IN_PROGRESS,
    COMMITTED,
    ABORTED
  };
  TxnID id;
  //timestamps, etc -- to be added
  WorkspacePtr wsp;
  State state;
  
  EmitQueue emitQ;
  EventQueue deltaQ, triggerQ;
  
  explicit TxnContext(TxnID i, WorkspacePtr w)
    : id(i)
    , wsp(w)
    , state(IN_PROGRESS)
    , emitQ()
    , deltaQ()
    , triggerQ()
  {}

};

class TxnManager {
private:
  WorkspacePtr curr_wsp;
  TxnID last_id;
public:
  explicit TxnManager(WorkspacePtr wsp)
    : curr_wsp(wsp)
    , last_id(0)
  {}
  
  TxnContext create();
  bool commit( TxnContext& );
  void abort( TxnContext& );
};

#endif

