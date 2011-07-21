/* $Id: event.hpp 225 2009-06-16 20:49:02Z maoy $ */

#ifndef EVENT_HPP_
#define EVENT_HPP_

#include <list>
#include <tr1/unordered_set>
#include <gc/gc_allocator.h>

#include "lib/types.hpp"

class Tuple;
class Address;

struct Event{
  enum EventType{
    NONE=0,
    RECV=1,
    INSERTED=2,
    DELETED=4,
    EXTRA = 8,
    REFRESHED=EXTRA,
    EXPIRED=EXTRA+1+DELETED,
    EVICTED=EXTRA+2+DELETED,
  };
  EventType type;
  Tuple* t;
  Event(EventType tp, Tuple* tt)
    :type(tp), t(tt){
  }
  ~Event(){}
};
std::ostream& operator<<(std::ostream& os, const Event& e);

class EventQueue{
private:
  std::list<Event, gc_allocator<Event> > _q;
  typedef std::tr1::unordered_set<ICommitable*
				  ,std::tr1::hash<ICommitable*>
				  ,std::equal_to<ICommitable*>
				  ,gc_allocator<ICommitable*> > table_set;
  table_set _tbls;
public:
  void enqueue(const Event& e){
    _q.push_front(e);
  }
  Event dequeue(){
    Event e = _q.back();
    _q.pop_back();
    return e;
  }
  bool empty() const{
    return _q.empty();
  }
  std::size_t size() const{
    return _q.size();
  }
  void touchTable(ICommitable* t){
    _tbls.insert( t );
  }
  void commitTables(){
    for( table_set::iterator it=_tbls.begin();
	 it!=_tbls.end();
	 ++it){
      (*it)->commit();
    }
    _tbls.clear();
  }
};

struct EmitEvent{
  Address* addr;
  Tuple* t;
};

class EmitQueue{
private:
  std::list<EmitEvent, gc_allocator<EmitEvent> > _q;
public:  
  void enqueue(const EmitEvent& e){
    _q.push_front(e);
  }
  EmitEvent dequeue(){
    EmitEvent e = _q.back();
    _q.pop_back();
    return e;
  }
  bool empty() const{
    return _q.empty();
  }
  std::size_t size() const{
    return _q.size();
  }
};
#endif
