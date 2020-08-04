cmake_minimum_required (VERSION 2.8)
project (SLOWProxy)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/*)
add_executable(udpliteping udpliteping.c)
