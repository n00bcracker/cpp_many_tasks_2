if (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/gen-rpc)
  return()
elseif (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/transport)
  return()
endif()

add_subdirectory(gen-rpc)

add_proto_library(cactus_rpc_test_proto rpc_test.proto)
add_proto_library(cactus_rpc_proto rpc.proto)

add_catch(test_cactus_rpc_gen gen-rpc/test.cpp)
target_link_libraries(test_cactus_rpc_gen PRIVATE cactus_rpc_test_proto)

add_shad_library(cactus_rpc transport/rpc.cpp)
target_link_libraries(cactus_rpc PRIVATE cactus_rpc_proto)

add_catch(test_cactus_rpc transport/test.cpp)
target_link_libraries(test_cactus_rpc PRIVATE cactus_rpc cactus_rpc_test_proto)
