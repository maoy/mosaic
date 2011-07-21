/* $Id: runtime.cpp 210 2009-04-14 22:14:22Z maoy $ */

#include <stdlib.h> //for exit
#include <signal.h> //signal handler

#include "runtime.hpp"

boost::asio::io_service iosv;

void int_handler(int sig) {
  printf("ctrl-C caught: exiting..\n");
  exit(1);
}

void setup_signal_handler(){
  signal(SIGINT, int_handler);
}
