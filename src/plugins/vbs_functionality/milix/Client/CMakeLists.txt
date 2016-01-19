cmake_minimum_required(VERSION 2.8.11)
project(VBSMilixClient)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5Widgets REQUIRED)
find_package(Qt5Network REQUIRED)

set(milixclient_SRC
  VBSMilixClient.cpp
)

set(milixclient_HDR
  VBSMilixClient.hpp
  ../VBSMilixCommands.hpp
)

add_definitions(-std=c++11)
add_definitions(-DVBSMILIXCLIENT_LIBRARY)

add_library(milixclient ${milixclient_SRC} ${milixclient_HDR})

target_link_libraries(milixclient Qt5::Gui Qt5::Network)
