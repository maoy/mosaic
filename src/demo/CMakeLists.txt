cmake_minimum_required (VERSION 2.6)

include_directories (${MOSAIC_SOURCE_DIR}/lib
  ${BOEHMGC_INCLUDE_DIR}
  ${Boost_INCLUDE_DIR} 
  ${MOSAIC_SOURCE_DIR} 
)
link_directories (${MOSAIC_BINARY_DIR}/lib ${Boost_LIBRARY_DIRS} )
#LINK_LIBRARIES(boost_serialization-gcc42-mt boost_system-gcc42-mt gc pthread)
LINK_LIBRARIES( ${Boost_SYSTEM_LIBRARY} 
  ${Boost_THREAD_LIBRARY} 
  ${GC_LIBRARY} 
  pthread rt)
add_executable (demo demo.cpp) 
add_executable (multicore multicore.cpp) 
add_executable (tunTest tunTest.cpp) 
add_executable (rawtun rawtun.cpp) 
add_executable (perf10 perf10.cpp) 
add_executable (count count.cpp) 
add_executable (bestpath bestpath.cpp) 
add_executable (db_bench db_bench.cpp) 
add_executable (txn txn.cpp) 

#add_executable (gctest gctest.cpp)
target_link_libraries (demo moslib)
target_link_libraries (multicore moslib)
target_link_libraries (tunTest moslib)
target_link_libraries (rawtun moslib)
target_link_libraries (perf10 moslib)
target_link_libraries (bestpath moslib)
target_link_libraries (db_bench moslib)
target_link_libraries (count moslib)
target_link_libraries (txn moslib)
