/* $Id: db_bench.cpp 210 2009-04-14 22:14:22Z maoy $ */

#include <iostream>

#include "table.hpp"
#include "refTable.hpp"
#include "softTable.hpp"
#include "time.hpp"
#include "s11n.hpp"

#include "poly_ptr.hpp"

const int n = 1000000;

//////begin tuple definitions//////////
enum {
  ID_Tuple_reserved = TUPLE_RESERVED,
  ID_Tuple_test,
  ID_Tuple_test2,
  ID_Tuple_test3,
  ID_Tuple_test4,
  ID_Tuple_last_id_
};
typedef tuple<String,gc_string,int> type_tuple_test;
DEFINE_TUPLE(test)

typedef tuple<gc_string,_gc_string,int> type_tuple_test2;
DEFINE_TUPLE(test2)

typedef tuple<gc_string,int> type_tuple_test3;
DEFINE_TUPLE(test3)

typedef tuple<int,int,int,int,int,int> type_tuple_test4;
DEFINE_TUPLE(test4)

//Tuple_test4 x(1,2,3,4,5); //this should not be compiled

/*
BEGIN_FROM_BUFFER_IMPL(NetIArchive)
INSTANCE_FROM_BUFFER_IMPL(test)
INSTANCE_FROM_BUFFER_IMPL(test2)
END_FROM_BUFFER_IMPL()
*/

void registerAllTuples(){
  REGISTER_TUPLE_ARCHIVE(test2,NetIArchive,NetOArchive)
  REGISTER_TUPLE_ARCHIVE(test,NetIArchive,NetOArchive)
}

/////end tuple definitions//////////

typedef RefTable<Tuple_test,
	      Keys<2>
	      //,Keys<1> 
	      > Table_test;


typedef SoftTable<Tuple_test,
	      Keys<2>
	      //,Keys<1> 
	      > Table_soft;

gc_string const1("asdf");
gc_string const2("bcd");
void hard(){
  std::cout << "benchmarking RefTable" << std::endl;
  Table_test t_test;
  PosixTime t1 = PosixTime::now();
  for (int i=0;i<n;i++){
    Tuple_test* t = new Tuple_test( const1, const2, i );
    t_test.insert(t);
  }
  t_test.commit();
  PosixTime t2 = PosixTime::now();
  std::cout << (t2-t1) << std::endl;
  std::cout << "table size:" << t_test.size() << std::endl;
  std::cout << "insertion per second:" 
            << n/(t2-t1).toDouble()
            << std::endl;
}

void soft(){
  std::cout << "benchmarking SoftTable" << std::endl;
  Table_soft t_test( TimeDuration::fromDouble(0.5) );
  PosixTime t1 = PosixTime::now();
  for (int i=0;i<n;i++){
    Tuple_test* t = new Tuple_test( const1, const2, i );
    t_test.insert(t);
  }
  t_test.commit();
  PosixTime t2 = PosixTime::now();
  std::cout << (t2-t1) << std::endl;
  std::cout << "size:" << t_test.size() << std::endl;
  std::cout << "insertion per second:" 
            << n/(t2-t1).toDouble()
            << std::endl;
}


void serialize()
{
  std::cout << "benchmarking serialization." << std::endl;
  Table_test t_test;
  PosixTime t1 = PosixTime::now();

  Tuple_test2* t = new Tuple_test2("asdf", "jkl", 10);
  Tuple_test2 res;
  for (int i=0;i<n;i++){
    /*
    NetOArchive nos;
    nos & *t;
    NetIArchive nis( * nos._buf);
    nis & res;
    */
    NetOArchive nos;
    Tuple::toBuffer(t, nos);
    NetIArchive nis( * nos._buf);
    Tuple::fromBuffer(nis);
  }
  //std::cout << res << std::endl;
  PosixTime t2 = PosixTime::now();
  std::cout << (t2-t1) << std::endl;
  std::cout << "serialization per second:" 
            << n/(t2-t1).toDouble()
            << std::endl;
}

void serialize2()
{
  

  SavePointerHelper<NetOArchive,Tuple>::getInstance()->registerClass<Tuple_test2>();
  SavePointerHelper<NetOArchive,Tuple>::getInstance()->registerClass<Tuple_test>();
  
  //RegisterClass<Tuple_test2, NetIArchive, NetOArchive> x;
  std::cout << "benchmarking serialization2." << std::endl;
  PosixTime t1 = PosixTime::now();

  Tuple_test2* t = new Tuple_test2("asdf", "jkl", 10);
  poly_ptr<Tuple> orig_ptr(t);
  const Tuple* res = NULL;
  for (int i=0;i<n;i++){
    NetOArchive oa;
    //int ii=10;
    //oa & t->id;
    //oa & *t;
    oa & orig_ptr;
    NetIArchive ia(*oa._buf);
    //res = LoadPointerHelper<NetIArchive,Tuple>::load(ID_Tuple_test2,ia);
    poly_ptr<Tuple> ptr;
    ia & ptr;
    res = & ptr.get();
  }
  PosixTime t2 = PosixTime::now();
  std::cout << *res << std::endl;
  std::cout << (t2-t1) << std::endl;
  std::cout << "serialization per second:" 
            << n/(t2-t1).toDouble()
            << std::endl;
}

int main(){
  registerAllTuples();
  serialize();
  serialize2();
  hard();
  soft();
  return 0;
}
