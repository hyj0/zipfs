cmake_minimum_required(VERSION 3.16)
project(zipfs)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 14)
include_directories("/cygdrive/e/Program Files (x86)/WinFsp/inc/")
link_directories("/cygdrive/e/Program Files (x86)/WinFsp/lib")

add_subdirectory(thirth_party/zip)


add_executable(zipfs main.cpp)
target_link_libraries(zipfs winfsp-x64 zip)

add_executable(testZip testZip.cpp)
target_link_libraries(testZip winfsp-x64 zip)