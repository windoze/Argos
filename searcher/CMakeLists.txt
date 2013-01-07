cmake_minimum_required(VERSION 2.8)

find_package(Log4CPlus REQUIRED)

add_executable(searcher searcher_main.cpp)
#TARGET_LINK_LIBRARIES(searcher net argos_ri_backend analyzer parser serialization query index common pugixml ${LIBS_MISC})
TARGET_LINK_LIBRARIES(searcher net parser argos_ri_backend parser json_spirit serialization query index analyzer common json_spirit pugixml ${LOG4CPLUS_LIBRARIES} ${LIBS_MISC} ${PROTOBUF_LIBRARY} pthread dl)
