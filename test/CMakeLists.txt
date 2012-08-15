cmake_minimum_required(VERSION 2.8)
#add_executable(test test_main.cpp)
add_executable(pb_reader pb_reader.cpp)
TARGET_LINK_LIBRARIES(pb_reader serialization ${LIBS_MISC})
