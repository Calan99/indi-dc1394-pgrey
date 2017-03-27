cmake_minimum_required(VERSION 2.4.7)
PROJECT(indi_dc1394_pgrey CXX C)

set(CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
set(BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin")

find_package(CFITSIO REQUIRED)
find_package(INDI)
find_package(ZLIB REQUIRED)
find_package(DC1394 REQUIRED)

include_directories( ${CMAKE_CURRENT_BINARY_DIR})
include_directories( ${CMAKE_CURRENT_SOURCE_DIR})
include_directories( ${INDI_INCLUDE_DIR})
include_directories( ${CFITSIO_INCLUDE_DIR})
include_directories( ${DC1394_INCLUDE_DIR})

set(dc1394_pgrey_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/indi_dc1394_pgrey.cpp)

add_executable(indi_dc1394_pgrey ${dc1394_pgrey_SRCS})

target_link_libraries(indi_dc1394_pgrey ${INDI_DRIVER_LIBRARIES} ${CFITSIO_LIBRARIES} ${DC1394_LIBRARIES} )

install(TARGETS indi_dc1394_pgrey RUNTIME DESTINATION bin )

install(FILES indi_dc1394_pgrey.xml DESTINATION ${INDI_DATA_DIR})

