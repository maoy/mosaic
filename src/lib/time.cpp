/* $Id: time.cpp 210 2009-04-14 22:14:22Z maoy $ */

#include "time.hpp"
#include "runtime.hpp"

using namespace boost::posix_time;
TimeDuration PosixTime::operator-(const PosixTime& rhs) const{
  if ( (!p) || (!rhs.p) ) // one of them is NULL
    return TimeDuration();
  time_duration* t = new (UseGC) time_duration();
  *t = get() - rhs.get();
  return( TimeDuration( t ));
}

/* TODO::
PosixTime PosixTime::operator+(const TimeDuration& p) const {
}
*/

PosixTime PosixTime::now(){
  return PosixTime( ::now() );
}

TimeDuration TimeDuration::operator+(const TimeDuration& rhs) const{
  if ( (!p) || (!rhs.p) ) // one of them is NULL
    return TimeDuration();
  time_duration* t = new (UseGC) time_duration();
  *t = get() + rhs.get();
  return( TimeDuration( t ));
}

TimeDuration TimeDuration::operator-(const TimeDuration& rhs) const{
  if ( (!p) || (!rhs.p) ) // one of them is NULL
    return TimeDuration();
  time_duration* t = new (UseGC) time_duration();
  *t = get() - rhs.get();
  return( TimeDuration( t ));
}

PosixTime TimeDuration::operator+(const PosixTime& rhs) const{
  if ( (!p) || (!rhs.p) ) // one of them is NULL
    return PosixTime();
  ptime* t = new (UseGC) ptime();
  *t = rhs.get()+get();
  return( PosixTime( t ));
}

TimeDuration TimeDuration::fromLong(long sec){
  return TimeDuration(new (UseGC) time_duration(seconds(sec)) );
}
TimeDuration TimeDuration::fromDouble(double sec){
  long micros = static_cast<long>(sec*1000000);
  return TimeDuration(new (UseGC) time_duration(microseconds(micros)) );
}
TimeDuration TimeDuration::fromMicroSec(int64_t micros){
  return TimeDuration( new (UseGC)
                       time_duration(microseconds(micros)) );
}

bool TimeDuration::operator>(double sec) const {
	return toDouble() > sec;
}
