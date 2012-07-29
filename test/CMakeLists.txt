cmake_minimum_required(VERSION 2.8)
add_executable(test test_main.cpp)
TARGET_LINK_LIBRARIES(test net parser argos_ri_backend parser serialization query index analyzer common pugixml ${LIBS_MISC})
