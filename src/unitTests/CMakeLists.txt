cmake_minimum_required (VERSION 2.6)

include_directories (${MOSAIC_SOURCE_DIR}/lib
  ${BOEHMGC_INCLUDE_DIR}
  ${Boost_INCLUDE_DIR} 
)

link_directories (${MOSAIC_BINARY_DIR}/lib ${Boost_LIBRARY_DIRS} /usr/local/lib) 

LINK_LIBRARIES( 
  moslib
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_THREAD_LIBRARY} 
  ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
  ${GC_LIBRARY}
  pthread 
  rt
)

ADD_EXECUTABLE(unitTest
  unitTest.cpp
  test_tuple.cpp
  test_table.cpp
)
TARGET_LINK_LIBRARIES(unitTest)
