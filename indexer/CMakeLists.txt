cmake_minimum_required(VERSION 2.8)
add_executable(indexer indexer_main.cpp)
TARGET_LINK_LIBRARIES(indexer analyzer parser argos_ri_backend analyzer query index common json_spirit pugixml ${LIBS_MISC} analyzer)
