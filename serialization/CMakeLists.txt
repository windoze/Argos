cmake_minimum_required(VERSION 2.8)
set(SOURCE_LIST)
file(GLOB H_LIST "*.h")
file(GLOB CPP_LIST "*.cpp")
list(APPEND SOURCE_LIST ${H_LIST} ${CPP_LIST})
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS result.proto)
add_library(serialization STATIC ${PROTO_SRCS} ${PROTO_HDRS} ${SOURCE_LIST})

