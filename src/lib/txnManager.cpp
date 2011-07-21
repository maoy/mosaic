/* $Id: txnManager.cpp 210 2009-04-14 22:14:22Z maoy $ */

#include <iostream>

#include "txnManager.hpp"

TxnContext TxnManager::create()
{
  TxnID newid = ++last_id;
  WorkspacePtr wsp = curr_wsp->clone();
  std::cerr << "creating txn: " << newid << std::endl;
  return TxnContext(newid,wsp);
}

bool TxnManager::commit(TxnContext& txn)
{
  //FIXME: check constraints
  assert( txn.state == TxnContext::IN_PROGRESS );
  curr_wsp = txn.wsp;
  txn.state = TxnContext::COMMITTED;
  return true;
}

void abort(TxnContext& txn)
{
  assert( txn.state == TxnContext::IN_PROGRESS );
  txn.state = TxnContext::ABORTED;
  txn.wsp = NULL; //discard the private workspace
}
