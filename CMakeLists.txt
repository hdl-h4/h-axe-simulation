cmake_minimum_required(VERSION 3.0.2)
project(h_axe_simulation)

set(CMAKE_CXX_STANDARD 17)

### glog
find_path(glog_INCLUDE_DIR NAMES glog/logging.h)
find_library(glog_LIBRARY NAMES glog)
if(glog_INCLUDE_DIR AND glog_LIBRARY)
    message(STATUS "Found glog:")
    message(STATUS "  (Headers)       ${glog_INCLUDE_DIR}")
    message(STATUS "  (Library)       ${glog_LIBRARY}")
else()
    message(ERROR "glog not found")
endif()

include_directories(${ZMQ_INCLUDE_DIR})
include_directories("include")

add_executable(Simulation src/main.cc)
target_link_libraries(Simulation ${AXE_EXTERNAL_LIB})
set_property(TARGET Simulation PROPERTY CXX_STANDARD ${CMAKE_CXX_STANDARD})
set_property(TARGET Simulation PROPERTY RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

