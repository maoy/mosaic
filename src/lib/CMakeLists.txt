#link_directories (/home/DBGROUP/maoy/third_party/lib) 

include_directories ( ${Boost_INCLUDE_DIR} 
         ${BOEHMGC_INCLUDE_DIR}
         ${MOSAIC_SOURCE_DIR}) 

LINK_LIBRARIES(${GC_LIBRARY} pthread)
ADD_LIBRARY (moslib 
  time.cpp
  table.cpp
  runtime.cpp 
  event.cpp 
  udp.cpp 
  packet.cpp
  tun.cpp 
  string.cpp
  csv.cpp
  address.cpp 
  workspace.cpp
  txnManager.cpp
)
