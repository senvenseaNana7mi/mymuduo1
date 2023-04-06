cmake_minimum_required(VERSION 2.5)
project(mymuduo)
#生成库文件
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")


aux_source_directory(. SRC_LIST)
add_library(mymuduo SHARED ${SRC_LIST})
