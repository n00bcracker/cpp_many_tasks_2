add_shad_library(libportknock portknock.cpp)
target_link_libraries(libportknock PUBLIC cactus)

add_catch(test_portknock test.cpp)
target_link_libraries(test_portknock PRIVATE libportknock)

add_shad_executable(run_portknock main.cpp)
target_link_libraries(run_portknock PRIVATE libportknock)
