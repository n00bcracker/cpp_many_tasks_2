add_shad_library(redis_server server.cpp simple_storage.cpp)
target_link_libraries(redis_server PUBLIC cactus resp_reader resp_writer)
target_include_directories(redis_server PUBLIC ..)

add_catch(test_redis_server test.cpp)
target_link_libraries(test_redis_server PRIVATE redis_server)
