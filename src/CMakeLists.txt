cmake_minimum_required (VERSION 2.6)
project (MOSAIC) 

#SET(CMAKE_MODULE_PATH "${CMAKE_ROOT}/Modules;${MOSAIC_SOURCE_DIR}/CMakeModules")
SET(CMAKE_MODULE_PATH "${MOSAIC_SOURCE_DIR}/CMakeModules")
#SET( Boost_ADDITIONAL_VERSIONS 1.37 )
SET( Boost_FIND_VERSION_MAJOR 1)
SET( Boost_FIND_VERSION_MINOR 37 )
SET( BOOST_INCLUDEDIR $ENV{HOME}/third_party/include )
SET( BOOST_LIBRARYDIR $ENV{HOME}/third_party/lib )
SET( Boost_USE_STATIC_LIBS ON )
SET( Boost_FIND_VERSION_EXACT ON )
SET( Boost_FIND_REQUIRED ON )
FIND_PACKAGE( Boost COMPONENTS date_time serialization system thread
unit_test_framework )

SET( BoehmGC_FIND_REQUIRED ON )
FIND_PACKAGE( BoehmGC )

INCLUDE (CheckIncludeFiles)
# usage: CHECK_INCLUDE_FILES (<header> <RESULT_VARIABLE> )
CHECK_INCLUDE_FILES ("linux/if_tun.h" HAVE_IF_TUN_H)
#CHECK_INCLUDE_FILES ("sys/param.h;sys/mount.h" HAVE_SYS_MOUNT_H)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

ADD_SUBDIRECTORY (lib)
ADD_SUBDIRECTORY (demo)
ADD_SUBDIRECTORY (unitTests)


