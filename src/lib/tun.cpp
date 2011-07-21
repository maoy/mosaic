/* $Id: tun.cpp 210 2009-04-14 22:14:22Z maoy $ */

#include <sys/types.h>
#include <sys/socket.h>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <linux/if_tun.h>

#include <boost/bind.hpp>

#include "event.hpp"
#include "string.hpp"
#include "tun.hpp"

/*
  allocate a linux tun device.
  when success: return 0, and set _fd, _dev_name
 */
int
TunDevice::try_linux_universal()
{
  int fd = open("/dev/net/tun", O_RDWR | O_NONBLOCK);
  if (fd < 0) {
    std::cerr << "open tun failed: " << strerror(errno) << std::endl;
    return -errno;
  }
  struct ifreq ifr;
  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = IFF_TUN | IFF_NO_PI; /*Do not provide packet information*/
  if (_dev_name.size()>0) {
    /* 
     * setting ifr_name this allows us to select an aribitrary 
     * interface name. 
     */
    strcpy(ifr.ifr_name, _dev_name.c_str());
  }
  int err = ioctl(fd, TUNSETIFF, (void *)&ifr);
  if (err < 0) {
    std::cerr << "Linux universal tun failed: " << strerror(errno);
    close(fd);
    return -errno;
  }

  _dev_name = ifr.ifr_name;
  std::cout << "sucessfully allocated " << _dev_name << std::endl;
  if (_dev_name == "tap0"){
    std::cout << "dev name: tap0. assuming PlanetLab" << std::endl;
    _type = TAP0; 
  }
  _fd_raw = fd;
  _fd.assign(_fd_raw);
  return 0;
}

int
TunDevice::setup_tun()
{
    char tmp[512];

    sprintf(tmp, "/sbin/ifconfig %s %s up 2>/dev/null", 
            _dev_name.c_str(), _if_cfg.c_str());
    std::cout << "executing " << tmp << std::endl;
    if (system(tmp) != 0) {
	// Is Ethertap available? If it is moduleified, then it might not be.
	// beside the ethertap module, you may also need the netlink_dev
	// module to be loaded.
      if (_type == TAP0){
        fprintf(stderr, "%s:%s ifconfig failed. expected on PlanetLab.\n", _dev_name.c_str(),tmp);
        //_mtu_out = 1468;
        //_mtu_in = _mtu_out +16;
        return 0;
      }
      std::cerr <<  _dev_name <<  "`" << tmp <<"' failed\n(Perhaps Ethertap is in a kernel module that you haven't loaded yet or bad ifconfig argument?)" << std::endl;
      return -1;
    }
    
#if 0
    if (_gw) {
#if defined(__linux__)
	sprintf(tmp, "/sbin/route -n add default gw %s", _gw.s().c_str());
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__APPLE__)
	sprintf(tmp, "/sbin/route -n add default %s", _gw.s().c_str());
#endif
	if (system(tmp) != 0)
	    return _errh->error("%s: %s", tmp, strerror(errno));
    }
#endif 
    // set MTU: now set directly with ifconfig
    /*    
#ifdef SIOCSIFMTU
    std::cout << "setting MTU to " << _mtu_out << std::endl;
    int s = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (s < 0) {
      std::cerr << "socket() failed: " << strerror(errno) << std::endl;
    }
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, _dev_name.c_str(), sizeof(ifr.ifr_name));
    ifr.ifr_mtu = _mtu_out;
    if (ioctl(s, SIOCSIFMTU, &ifr) != 0)
      std::cerr << "SIOCSIFMTU failed: " << strerror(errno) << std::endl;
    close(s);
#endif
    
    // calculate maximum packet size needed to receive data from
    // tun/tap.
    if (_type == LINUX_UNIVERSAL)
	_mtu_in = _mtu_out + 4;
    else if (_type == BSD_TUN)
	_mtu_in = _mtu_out + 4;
    else if (_type == OSX_TUN)
	_mtu_in = _mtu_out + 4; // + 0?
    else // _type == LINUX_ETHERTAP 
	_mtu_in = _mtu_out + 16;
    */
    return 0;
}

int TunDevice::configure(const gc_string& cfg){
  //_mtu_out = DEFAULT_MTU;
  /*
  string_vector v = string_split(cfg);
  assert( v.size() >=1 );
  _near = v[0];
  _mask = v[1];
  if (v.size()>2)
    _mtu_out = atoi( v[2].c_str() );
  */
  _if_cfg = cfg;
  return 0;
}

int 
TunDevice::init()
{
  if (try_linux_universal() < 0) 
    throw std::runtime_error("cannot open tun device. need root permission?");
  return setup_tun();
}

/*
void TunDevice::enableRead(){
  if (_buf) //a read is already pending
    return;
  _buf = new (UseGC) gc_streambuf();
  _fd.async_read_some(_buf->prepare(1500),
                          boost::bind(&TunDevice::onRead, this,
                                      boost::asio::placeholders::error,
                                      boost::asio::placeholders::bytes_transferred));
}

extern void demux(Event e);

void TunDevice::onRead(const boost::system::error_code& error,
                    std::size_t len)
{
  if (!error || error == boost::asio::error::message_size) {
    _buf->commit(len);
    Tuple *t = new Tuple_tun( Packet(_buf));
    _buf = NULL;
    Event ev(Event::RECV, t);
    demux(ev);
    enableRead();
  }else{
    std::cerr << "error reading from socket" << std::endl;
  }
}
*/
