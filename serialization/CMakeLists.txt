cmake_minimum_required(VERSION 2.8)
set(SOURCE_LIST)
file(GLOB H_LIST "*.h")
file(GLOB CPP_LIST "*.cpp")
list(APPEND SOURCE_LIST ${H_LIST} ${CPP_LIST})
add_library(serialization STATIC ${SOURCE_LIST})

