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

project(axe_simulation)
cmake_minimum_required(VERSION 3.0.2)

set(CMAKE_CXX_STANDARD 17)

### Dependencies
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(dep)

### Source
include_directories(${PROJECT_SOURCE_DIR}/src ${AXE_SIMULATION_EXTERNAL_INCLUDES})

FILE(GLOB simulation-sources
  src/algorithm/algorithm_book.cc
  src/algorithm/task_placement.cc
  src/scheduler/scheduler_impl.cc
  src/event/event.cc
)

add_library(simulation-obj OBJECT ${simulation-sources})
set_property(TARGET simulation-obj PROPERTY CXX_STANDARD ${CMAKE_CXX_STANDARD})
if(TARGET json_ep)
    add_dependencies(simulation-obj json_ep)
endif()

add_library(static-simulation STATIC $<TARGET_OBJECTS:simulation-obj>)
target_link_libraries(static-simulation ${AXE_SIMULATION_EXTERNAL_LIB})

### Executable
add_executable(Simulation src/main.cc)

target_link_libraries(Simulation static-simulation ${AXE_SIMULATION_EXTERNAL_LIB})
set_property(TARGET Simulation PROPERTY CXX_STANDARD ${CMAKE_CXX_STANDARD})
set_property(TARGET Simulation PROPERTY RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

#Tests
include(CTest)
add_subdirectory(test)
