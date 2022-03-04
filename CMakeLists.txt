cmake_minimum_required(VERSION 3.21)
project(homework)

set(CMAKE_CXX_STANDARD 20)


#find_package(SQLite3 REQUIRED)
#find_package(Boost REQUIRED)
#include_directories(${SQLite3_INCLUDE_DIRS})
#include_directories(${Boost_INCLUDE_DIRS})
include_directories("include/boost")
include_directories("include/sqlite")
add_executable(homework main.cpp Account.h Transaction.h)
link_directories("libs")
target_link_libraries (homework sqlite3)