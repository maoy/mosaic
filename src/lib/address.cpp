/* $Id: address.cpp 210 2009-04-14 22:14:22Z maoy $ */

//#include "address.hpp"
#include <stdlib.h>
#include "types.hpp"

int parseAddr(const char* addr, gc_string& ip, uint16_t& port){
  const char* theString = addr;
  char* theColonSign = strchr(theString, ':');
  if (theColonSign == NULL) {
    std::cerr << "address is malformed: " << addr << std::endl;
    throw std::runtime_error("malformed address");
    //return -1;
  }
  ip = gc_string(theString, theColonSign - theString);
  gc_string strPort = gc_string(theColonSign + 1);
  port = atoi(strPort.c_str());
  return 0;
}
