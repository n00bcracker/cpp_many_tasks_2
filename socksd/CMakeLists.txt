add_shad_library(libsocksd socksd.cpp)
target_link_libraries(libsocksd PUBLIC cactus)

add_catch(test_socksd test.cpp)
target_link_libraries(test_socksd PRIVATE libsocksd)

add_shad_executable(run_socksd main.cpp)
target_link_libraries(run_socksd PRIVATE libsocksd)
