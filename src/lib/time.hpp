/* $Id: time.hpp 211 2009-04-14 22:16:01Z maoy $ */

#ifndef TIME_HPP_
#define TIME_HPP_

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lexical_cast.hpp>

#include "types.hpp"

class TimeDuration;
class PosixTime: public gc_ptr<boost::posix_time::ptime>{
public:
  //constructors
  friend class TimeDuration;
  PosixTime()
    : gc_ptr<boost::posix_time::ptime>() {}
  PosixTime(const PosixTime& t)
    : gc_ptr<boost::posix_time::ptime>(t.p) {}
  PosixTime(boost::posix_time::ptime* p)
    : gc_ptr<boost::posix_time::ptime>(p) {}
  PosixTime(boost::posix_time::ptime p)
    : gc_ptr<boost::posix_time::ptime>(p) {}
  /*
  template <class Archive>
  void serialize(Archive& ar, unsigned int version){
    throw std::logic_error("time serialization not implemented");
  }
  */
  //static functions
  static PosixTime now();

  //operators
  TimeDuration operator-(const PosixTime& p) const;
  PosixTime operator+(const TimeDuration& p) const;
  
};
class TimeDuration: public gc_ptr<boost::posix_time::time_duration> {
public:
  //constructors
  TimeDuration()
    : gc_ptr<boost::posix_time::time_duration>() {}
  TimeDuration(const TimeDuration& t)
    : gc_ptr<boost::posix_time::time_duration>(t.p) {}
  TimeDuration(boost::posix_time::time_duration* p)
    : gc_ptr<boost::posix_time::time_duration>(p) {}
  TimeDuration(boost::posix_time::time_duration p)
    : gc_ptr<boost::posix_time::time_duration>(p) {}

  /*template <class Archive>
  void serialize(Archive& ar, unsigned int version){
    throw std::logic_error("time duration serialization not implemented");
    }*/
  //type conversions
  static TimeDuration fromLong(long sec);
  static TimeDuration fromDouble(double sec);
  static TimeDuration fromMicroSec(int64_t micros);
  
  //operators
  TimeDuration operator+(const TimeDuration& p) const;
  TimeDuration operator-(const TimeDuration& p) const;
  PosixTime operator+(const PosixTime& p) const;
  double toDouble() const{
    return double(get().total_microseconds())/1000000.0;
  };

  bool operator>(double sec) const;
  
};

template <>
inline TimeDuration from_string_cast(const char* s)
{
  return TimeDuration::fromDouble(boost::lexical_cast<double>(s));
}

#endif
