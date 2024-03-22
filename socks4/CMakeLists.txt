add_shad_library(libsocks4 socks4.cpp)
target_link_libraries(libsocks4 PUBLIC cactus)

add_catch(test_socks4 test.cpp)
target_link_libraries(test_socks4 PRIVATE libsocks4)

add_shad_executable(run_socks4 main.cpp)
target_link_libraries(run_socks4 PRIVATE libsocks4)
