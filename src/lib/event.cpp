/* $Id: event.cpp 210 2009-04-14 22:14:22Z maoy $ */

#include <iostream>

#include "event.hpp"
#include "tuple.hpp"

const char* eventTypeName[] = {"Recv","Inserted","Refreshed","Deleted"};

std::ostream& operator<<(std::ostream& os, const Event& e){
  os << "Event(" << eventTypeName[e.type] << "," << *e.t << ")";
  return os;
}

