cmake_minimum_required(VERSION 3.16)
project(zipfs)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 14)
include_directories("/cygdrive/e/Program Files (x86)/WinFsp/inc/")
link_directories("/cygdrive/e/Program Files (x86)/WinFsp/lib")

add_subdirectory(thirth_party/zip)

include_directories(thirth_party/uthread)
add_library(uthread thirth_party/uthread/pd_log.c  thirth_party/uthread/pd_uthread.c)
add_executable(test_uthread thirth_party/uthread/test_uthread.c)
target_link_libraries(test_uthread uthread)

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    MESSAGE(STATUS "current platform: Linux ")
    add_executable(zipfs main.cpp ZipRead.cpp)
    target_link_libraries(zipfs fuse3 zip uthread)

    add_executable(testZip testZip.cpp)
    target_link_libraries(testZip fuse3 zip uthread)
else()
    MESSAGE(STATUS "current platform: Windows")
    add_executable(zipfs main.cpp ZipRead.cpp)
    target_link_libraries(zipfs winfsp-x64 zip uthread)

    add_executable(testZip testZip.cpp)
    target_link_libraries(testZip winfsp-x64 zip uthread)
endif()