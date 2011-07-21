/* $Id: queue.hpp 211 2009-04-14 22:16:01Z maoy $ */

#ifndef BOOST_THREAD_QUEUE_H
#define BOOST_THREAD_QUEUE_H

#include <queue>
#include <deque>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/xtime.hpp>

template
  <
    typename T,
    //typename Q = std::priority_queue< boost::shared_ptr< T > >
    typename Q = std::queue<T, std::deque<T, gc_allocator<T> > >
    >
  class Queue
  {
  public:
    typedef Q queue_type;
    //typedef boost::shared_ptr< T >			msg_type;
    typedef T msg_type;
    typedef typename queue_type::size_type	size_type;
		
  private:
    queue_type				queue_;
    boost::mutex			mtx_;
    boost::condition		not_empty_cond_;
    boost::condition		not_full_cond_;
    size_type				high_water_mark_;
    size_type				low_water_mark_;
    bool					active_;
    static const size_type	infinity_;

    void enqueue_( msg_type const& msg)
    { queue_.push( msg); }
		
    void dequeue_( msg_type & msg)
    {
      msg = queue_.front();
      queue_.pop();
    }
		
    bool full_() const
    {
      if ( high_water_mark_ == infinity_) return false;
      return queue_.size() > high_water_mark_;
    }
		
    bool empty_() const
    { return queue_.empty(); }
		
    bool suppliers_activate_() const
    { return active_ == false || ! full_(); }
		
    bool consumers_activate_() const
    { return active_ == false || ! empty_(); }
		
    void activate_()
    {
      active_ = true;
    }
		
    void deactivate_()
    {
      if ( active_)
	{
	  active_ = false;
	  not_empty_cond_.notify_all();
	  not_full_cond_.notify_all();
	}
    }
		
    void flush_()
    { queue_.clear(); }
		
  public:
    Queue()
      : 
      queue_(), 
      mtx_(), 
      not_empty_cond_(), 
      not_full_cond_(), 
      high_water_mark_( infinity_),
      low_water_mark_( infinity_),
      active_( true)
    {}
		
    Queue( size_type water_mark)
      : 
      queue_(), 
      mtx_(), 
      not_empty_cond_(), 
      not_full_cond_(), 
      high_water_mark_( water_mark),
      low_water_mark_( water_mark),
      active_( true)
    {}
		
    Queue( 
	  size_type low_water_mark, 
	  size_type high_water_mark)
      : 
      queue_(), 
      mtx_(), 
      not_empty_cond_(), 
      not_full_cond_(), 
      high_water_mark_( high_water_mark),
      low_water_mark_( low_water_mark),
      active_( true)
    {}
		
    void activate()
    {
      typename boost::mutex::scoped_lock lock( mtx_);
      activate_();
    }
		
    void deactivate()
    {
      typename boost::mutex::scoped_lock lock( mtx_);
      deactivate_();
    }
		
    bool empty()
    { 
      typename boost::mutex::scoped_lock lock( mtx_);
      return empty_(); 
    }
		
    bool full()
    {
      typename boost::mutex::scoped_lock lock( mtx_);
      return full_();
    }
		
    bool enqueue( msg_type const& msg)
    {
      typename boost::mutex::scoped_lock lock( mtx_);
      if ( active_ == false) return false;
      not_full_cond_.wait( 
			  lock,
			  boost::bind(
				      & Queue< T, Q >::suppliers_activate_,
				      this) );
      if ( active_ != false)
	{
	  enqueue_( msg);
	  not_empty_cond_.notify_one();
	  return true;
	}
      else
	return false;
    }
		
    bool enqueue( msg_type const& msg, boost::xtime const& xt)
    {
      typename boost::mutex::scoped_lock lock( mtx_);
      if ( active_ == false) return false;
      not_full_cond_.timed_wait( 
				lock,
				xt,
				boost::bind(
					    & Queue< T, Q >::suppliers_activate_,
					    this) );
      if ( active_ != false)
	{
	  enqueue_( msg);
	  not_empty_cond_.notify_one();
	  return true;
	}
      else
	return false;
    }
		
    bool dequeue( msg_type & msg)
    {
      typename boost::mutex::scoped_lock lock( mtx_);
      if ( active_ == false && empty_() ) return false;
      while (empty_() ){
	not_empty_cond_.wait( 
			     lock,
			     boost::bind(
					 & Queue< T, Q >::consumers_activate_,
					 this) );
      }
      dequeue_( msg);
      if ( active_ == true && queue_.size() <= low_water_mark_) {
	
	not_full_cond_.notify_one();
      }
      return msg ? true : false;
    }
		
    bool dequeue( msg_type & msg, boost::xtime const& xt)
    {
      typename boost::mutex::scoped_lock lock( mtx_);
      if ( active_ == false && empty_() ) return false;
      not_empty_cond_.timed_wait( 
				 lock,
				 xt,
				 boost::bind(
					     & Queue< T, Q >::consumers_activate_,
					     this) );
      if ( empty_() )
	msg.reset();
      else
	dequeue_( msg);
      if ( active_ == true && queue_.size() <= low_water_mark_)
	not_full_cond_.notify_one();
      return msg ? true : false;
    }
    std::size_t size() const{
      return queue_.size();
    }
		
    void flush()
    {
      typename boost::mutex::scoped_lock lock( mtx_);
      flush_();
    }
		
    void close()
    {
      typename boost::mutex::scoped_lock lock( mtx_);
      deactivate_();
      flush_();
    }
  };
	
template
<
  typename T,
  typename Q
  >
const typename Queue< T, Q >::size_type
Queue< T, Q >::infinity_ = -1;


#endif // BOOST_THREAD_QUEUE_H
