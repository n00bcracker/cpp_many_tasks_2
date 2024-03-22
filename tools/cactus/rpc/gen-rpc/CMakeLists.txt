add_shad_executable(protoc-gen-rpc main.cpp SOLUTION_SRCS rpc_generator.cpp)
target_link_libraries(protoc-gen-rpc PRIVATE protobuf protoc)
