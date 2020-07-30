cmake_minimum_required(VERSION 2.8)
project(SLOWProxy C)
set(CMAKE_C_COMPILER "clang")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -g3 -Wall -pedantic -Wno-address -Wno-unused-function -Wno-empty-translation-unit")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Ofast -Wall")
file(GLOB_RECURSE ALL_DOTC LIST_DIRECTORIES false RELATIVE ${PROJECT_SOURCE_DIR} *.c)


file(GLOB_RECURSE UNIVERSE LIST_DIRECTORIES true RELATIVE ${PROJECT_SOURCE_DIR} *)
SET(SUBDIRS "")
FOREACH (SUBDIR ${UNIVERSE})
    IF (IS_DIRECTORY ${PROJECT_SOURCE_DIR}/${SUBDIR})
        include_directories(${SUBDIR})
    ENDIF ()
ENDFOREACH ()

add_executable(slowproxy ${ALL_DOTC} ${ALL_DOTH})

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_search_module(gpgme required)
pkg_search_module(libuv required)
pkg_search_module(libmnl required)
add_library(libnetlink STATIC IMPORTED)

target_link_libraries(slowproxy ${OpenSSL_LIBRARIES})
target_link_libraries(slowproxy ${Threads_LIBRARIES})
target_link_libraries(slowproxy ${gpgme_LIBRARIES})
target_link_libraries(slowproxy ${libuv_LIBRARIES})
target_link_libraries(slowproxy ${libmnl_LIBRARIES})
target_link_libraries(slowproxy libnetlink.a)

set(CMAKE_VERBOSE_MAKEFILE off)