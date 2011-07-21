/* $Id: s11n.hpp 211 2009-04-14 22:16:01Z maoy $ */

#ifndef S11N_HPP__
#define S11N_HPP__

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>
#include <tr1/unordered_map>

#include "types.hpp"
#include "vector.hpp"

//#include "tuple.hpp"

typedef std::vector<boost::asio::const_buffer, 
                    gc_allocator<boost::asio::const_buffer> > buffer_vector;

template<class T, class IArchive>
T* load_pointer(IArchive& ia){
  T* res = new T();
  ia & *res;
  return res;
}
template<class BaseT, class T, class IArchive>
void save_pointer(BaseT* t, IArchive& ia){
  ia & * static_cast<T*>(t);

}

template <class Archive, class BaseT>
class SavePointerHelper
  :public Singleton< SavePointerHelper<Archive, BaseT> > 
{
public:
  typedef boost::function<void (BaseT*, Archive& )> save_func;
  // T is a subclass of BaseT
  template <class T> 
  void registerClass(){
    _savemap[T::TypeID] = save_pointer<BaseT, T, Archive>;    
  }
  void save(uint16_t id, BaseT* t, Archive& ia){
    return _savemap[id](t, ia);
  }
 private:
  std::tr1::unordered_map<uint16_t, save_func> _savemap;
};

template <class Archive, class BaseT>
class LoadPointerHelper
  :public Singleton< LoadPointerHelper<Archive, BaseT> >{
public:
  typedef boost::function<BaseT* ( Archive& )> load_func;
  // T is a subclass of BaseT
  template <class T> 
  void registerClass(){
    _loadmap[T::TypeID] = load_pointer<T,Archive>;
    
  }
  BaseT* load(uint16_t id,Archive& ia){
    return _loadmap[id](ia);
  }
private:
  std::tr1::unordered_map<uint16_t, load_func> _loadmap;
};

#define SAVE_RAW(type)                           \
  inline std::size_t operator &(const type & t){ \
    return save_raw(t);				 \
  }

class NetOArchive{
public:
  typedef boost::mpl::bool_<false> is_loading;
  typedef boost::mpl::bool_<true> is_saving;
  typedef boost::asio::const_buffers_1 const_buffers_type;
private:
  bool _finalized;
  void finalize(){
    if (_finalized) return;
    _bv.push_back(_buf->data());
    _finalized = true;
    _buf = NULL;
  }
public:
  NetOArchive():_finalized(false), _bv(){
    _buf = new (UseGC) gc_streambuf();
  }

  buffer_vector* data() {
    //std::cout << "data with len=" << _buf->size() << std::endl;
    finalize();
    return &_bv;
  }

  template<class T>
  std::size_t save_raw(const T& t){
    T* p = boost::asio::buffer_cast<T*>(_buf->prepare(sizeof(T)));
    *p = t;
    _buf->commit(sizeof(T));
    return sizeof(T);
  }
  SAVE_RAW(int16_t)
  SAVE_RAW(int32_t)
  SAVE_RAW(int64_t)
  SAVE_RAW(bool)
  SAVE_RAW(uint16_t)
  SAVE_RAW(uint32_t)
  SAVE_RAW(uint64_t)
  SAVE_RAW(char)
  SAVE_RAW(double)

  std::size_t operator &(const _gc_string& s){
    length_t sz = s.size();
    std::size_t ret = *this & sz;
    _buf->sputn(s.c_str(), sz);
    /*
    char* p = boost::asio::buffer_cast<char*>(_buf->prepare(sz));
    memcpy(p, s.c_str(), sz);
    _buf->commit(sz);
    */
    ret += sz;
    return ret;
  }

  std::size_t operator&(gc_streambuf& buf){
    length_t len = buf.size();
    std::size_t ret = *this & len;
    if (len > 0){
#if 0 //memory copy 
      char* p = boost::asio::buffer_cast<char*>(_buf->prepare((len) ));
      const char* src = boost::asio::buffer_cast<const char*>(buf.data());
      memcpy( p, src, len);
      ret += len;
      _buf->commit( len );
#else //use iovec
      _bv.push_back(_buf->data());
      _buf = &buf;
#endif
    }
    return ret;
  }
  //deque
  template<class T>
  std::size_t operator&(const std::deque<T,gc_allocator<T> >& s){ 
    length_t sz = s.size();
    std::size_t ret = *this & sz;
    for (typename std::deque<T,gc_allocator<T> >::const_iterator it=s.begin();
	 it!=s.end();
	 ++it){
      ret += *this & (*it);
    }
    return ret;
  }
  template<class T>
  std::size_t operator &(const T& t){
    const_cast<T&>(t).serialize(*this, 1);
    return 0;
  }
  
  //time_duration: convert to microsecond count
  std::size_t operator&(const boost::posix_time::time_duration& s){
    const int64_t t = s.total_microseconds();
    std::size_t ret = *this & t;
    return ret;
  }
  //ptime: convert to time_duration since epoch
  std::size_t operator&(const boost::posix_time::ptime& t){
    using namespace boost::posix_time;
    using namespace boost::gregorian;
    typedef boost::date_time::c_local_adjustor<ptime> local_adj;
    const static ptime epoch_UTC(date(1970,1,1));
    const static ptime epoch_local = local_adj::utc_to_local(epoch_UTC);
    const time_duration td = t - epoch_local;
    return *this & td;
  }

  template <class T>
  std::size_t operator<<(const T& t){
    return (*this) & t;
  }
  gc_streambuf* _buf;
  buffer_vector _bv;
};

#define LOAD_RAW(type)			   \
  inline std::size_t operator &(type & t){ \
    return load_raw(t);			   \
  }

class NetIArchive{
public:
  typedef boost::mpl::bool_<true> is_loading;
  typedef boost::mpl::bool_<false> is_saving;
  NetIArchive(gc_streambuf& buf):_buf(&buf){
  }
  template<class T>
  std::size_t load_raw(T& t){
    if (sizeof(T) > _buf->size() ){
      throw std::runtime_error("bad serialization when loading raw type");
    }
    const T* p = boost::asio::buffer_cast<const T*>( _buf->data() );
    t = *p;
    _buf->consume(sizeof(T) );
    return sizeof(T);
  }
  LOAD_RAW(int16_t)
  LOAD_RAW(int32_t)
  LOAD_RAW(int64_t)
  LOAD_RAW(bool)
  LOAD_RAW(uint16_t)
  LOAD_RAW(uint32_t)
  LOAD_RAW(uint64_t)
  LOAD_RAW(double)
  LOAD_RAW(char)

  std::size_t operator &(_gc_string& s){
    std::size_t ret;
    length_t sz;
    ret = *this & sz;
    if ( sz>0 ) {
      
      if (sz > _buf->size() ){
	throw std::runtime_error("bad serialization when loading string");
      }
      s.resize(sz);
      ret += _buf->sgetn( &(*s.begin()) , sz );
    }
    return ret;
  }

  template<class T>
  std::size_t operator&(std::deque<T,gc_allocator<T> >& x){
    length_t len;
    std::size_t ret = *this & len;
    x.clear();
    //x.reserve(len);
    for (length_t i=0; i < len; ++i){
      T value;
      ret += *this & value;
      x.push_back(value);
    }
    return ret;
  }

  std::size_t operator&(gc_streambuf& buf){
    throw std::logic_error("should have been optmized out!");
    length_t len;
    std::size_t ret = *this & len;
    if (len > 0){
      if ( len > _buf->size() ){
	throw std::runtime_error("bad serialization when loading streambuf");
      }
      char* p = boost::asio::buffer_cast<char*>(buf.prepare((len) ));
      ret += _buf->sgetn( p, len );
      buf.commit( len );
    }
    return ret;
  }

  //time_duration
  std::size_t operator&(boost::posix_time::time_duration& s){
    int64_t t=0;
    std::size_t ret = *this & t;
    s = boost::posix_time::time_duration( boost::posix_time::microseconds(t) );
    return ret;
  }
  
  //ptime
  std::size_t operator&(boost::posix_time::ptime& t){
    using namespace boost::posix_time;
    using namespace boost::gregorian;
    typedef boost::date_time::c_local_adjustor<ptime> local_adj;
    const static ptime epoch_UTC(date(1970,1,1));
    const static ptime epoch_local = local_adj::utc_to_local(epoch_UTC);
    time_duration td;
    std::size_t ret = *this & td;
    t = epoch_local + td;
    return ret;
  }

  template<class T>
  std::size_t operator &(T& t){
    t.serialize(*this, 1);
    return 0;
  }

  template <class T>
  std::size_t operator>>(T& t){
    return (*this) & t;
  }

  gc_streambuf* _buf;
};
#endif
