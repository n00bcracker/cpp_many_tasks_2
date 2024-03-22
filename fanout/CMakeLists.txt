add_shad_library(libfanout fanout.cpp)
target_link_libraries(libfanout PUBLIC cactus)

add_catch(test_fanout test.cpp)
target_link_libraries(test_fanout PRIVATE libfanout)

add_shad_executable(run_fanout main.cpp)
target_link_libraries(run_fanout PRIVATE libfanout)
