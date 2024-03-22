add_subdirectory(gen-rpc)

add_proto_library(cactus_rpc_test_proto rpc_test.proto)
add_proto_library(cactus_rpc_proto rpc.proto)

add_catch(test_cactus_rpc_gen gen-rpc/test.cpp)
target_link_libraries(test_cactus_rpc_gen PRIVATE cactus_rpc_test_proto)

add_catch(test_cactus_rpc test.cpp SOLUTION_SRCS rpc.cpp)
target_link_libraries(test_cactus_rpc PRIVATE cactus_rpc_test_proto cactus_rpc_proto)
