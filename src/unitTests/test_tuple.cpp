/* $Id: test_tuple.cpp 210 2009-04-14 22:14:22Z maoy $ */

#include <boost/test/unit_test.hpp>

#include "tuple.hpp"

enum {
  ID_unused = TUPLE_RESERVED
  ,ID_Tuple_test1,
  ID_Tuple_test2
};
typedef tuple< int, char, gc_string> type_tuple_test1;
DEFINE_TUPLE( test1 )

typedef tuple< int, uint64_t, int, int, int, int, int, int, int > type_tuple_test2;
DEFINE_TUPLE( test2 )

BOOST_AUTO_TEST_SUITE( tuple_suite )

BOOST_AUTO_TEST_CASE( tuple_creation )
{
  Tuple_test1 t(1, 'c', "stuff");
  BOOST_CHECK( 1==t.get<0>() );
  BOOST_CHECK_MESSAGE( t.get<2>()=="stuff", "string problem");  
}
BOOST_AUTO_TEST_CASE( tuple_comparison )
{
  {
    Tuple_test1 t1(1,'c',"stuff1"), t2(1,'c',"stuff2");
    BOOST_CHECK( t1<t2);  
  }
  {
    Tuple_test1 t1(1,'c',"stuff1"), t2(1,'c',"stuff1");
    BOOST_CHECK( t1==t2);  
  }
  {
    Tuple_test2 t1(1,2,3,4,5,6,7,8,9), t2;
    BOOST_CHECK( t1>t2);
  }
}

BOOST_AUTO_TEST_CASE( tuple_projection )
{
  Tuple_test1 t1(1,'a',"test1");
  TupleProjection<Tuple_test1, Keys<1> > proj;
  BOOST_CHECK_MESSAGE( *proj(&t1)=='a', "Projection to single field" );
  TupleProjection<Tuple_test1, Keys<0,1,2> > proj2;
  BOOST_CHECK_MESSAGE( *proj2(&t1)==t1, "Projection to all fields" );

  Tuple_test2 t2(1,2,3,4,5,6,7,8,9);
  TupleProjection<Tuple_test2, Keys<2,3,4,5,6> > proj3;
  BOOST_CHECK( proj3(&t2)!=NULL);

  //self projection
  TupleProjection<Tuple_test2, Keys<> > proj4;
  BOOST_CHECK( *proj4(&t2)==t2);
}
BOOST_AUTO_TEST_SUITE_END()
