# Copyright 2018 H-AXE
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

include(ExternalProject)

### gflgas
find_package(gflags REQUIRED)
message(STATUS "Found gflags:")
message(STATUS "  (Headers)       ${gflags_INCLUDE_DIR}")
message(STATUS "  (Library)       ${gflags_LIBRARIES}")
list(APPEND AXE_SIMULATION_EXTERNAL_INCLUDES ${gflags_INCLUDE_DIR})
list(APPEND AXE_SIMULATION_EXTERNAL_LIB ${gflags_LIBRARIES})

### glog
find_path(glog_INCLUDE_DIR NAMES glog/logging.h)
find_library(glog_LIBRARY NAMES glog)
if(glog_INCLUDE_DIR AND glog_LIBRARY)
    message(STATUS "Found glog:")
    message(STATUS "  (Headers)       ${glog_INCLUDE_DIR}")
    message(STATUS "  (Library)       ${glog_LIBRARY}")
    list(APPEND AXE_SIMULATION_EXTERNAL_INCLUDES ${glog_INCLUDE_DIR})
    list(APPEND AXE_SIMULATION_EXTERNAL_LIB ${glog_LIBRARY})
else()
    message(ERROR "glog not found")
endif()

### json
set(JSON_VERSION "3.4.0")
set(THIRDPARTY_DIR ${PROJECT_BINARY_DIR}/third_party)

if (NOT ("${JSON_INCLUDE_DIR}" STREQUAL "JSON_INCLUDE_DIR-NOTFOUND" OR "${JSON_INCLUDE_DIR}" STREQUAL ""))
  file(TO_CMAKE_PATH ${JSON_INCLUDE_DIR} _json_path)
else()
  file(TO_CMAKE_PATH ${THIRDPARTY_DIR} _json_path)
endif()

find_path (JSON_INCLUDE_DIR NAMES "nlohmann/json.hpp" HINTS ${_json_path})

if (JSON_INCLUDE_DIR)
  message (STATUS "Found nlohmann/json.hpp:")
  message (STATUS " (Headers)   ${JSON_INCLUDE_DIR}")
  list(APPEND AXE_SIMULATION_EXTERNAL_INCLUDES ${JSON_INCLUDE_DIR})
else()
  message (STATUS "add external project nlohmann-json")
  ExternalProject_Add(json_ep
    PREFIX ${THIRDPARTY_DIR}/nlohmann
    DOWNLOAD_DIR ${THIRDPARTY_DIR}/nlohmann
    DOWNLOAD_NO_EXTRACT true
    SOURCE_DIR ""
    BINARY_DIR ""
    URL "https://github.com/nlohmann/json/releases/download/v${JSON_VERSION}/json.hpp"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    ${THIRDPARTY_LOG_OPTIONS}) 
  message(STATUS "Thirdparti_dir ${THIRDPARTY_DIR}")
  list(APPEND AXE_SIMULATION_EXTERNAL_INCLUDES ${THIRDPARTY_DIR})
endif()

include_directories(${AXE_SIMULATION_EXTERNAL_INCLUDES})
