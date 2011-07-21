/* $Id: runtime.hpp 211 2009-04-14 22:16:01Z maoy $ */

#ifndef RUNTIME_HPP_
#define RUNTIME_HPP_

#include <gc/gc_cpp.h>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

extern boost::asio::io_service iosv;
using boost::asio::deadline_timer;

template<typename WaitHandler>
void callLater(int sec, const WaitHandler& h, boost::asio::deadline_timer* tm=NULL){
  if (sec==0) return iosv.post(h);
  //std::cerr << "in callLater: will call later in "<<sec << "secs\n";
  if (tm){
    tm->expires_from_now(boost::posix_time::seconds(sec));
    tm->async_wait(h);
  }else{
    boost::asio::deadline_timer*
      t = new (UseGC) boost::asio::deadline_timer(iosv, 
                                                  boost::posix_time::seconds(sec));
    t->async_wait(h);
  }
}

inline boost::posix_time::ptime now(){
  return boost::posix_time::microsec_clock::local_time();
}

void setup_signal_handler();

#endif
