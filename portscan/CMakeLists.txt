add_shad_library(libportscan portscan.cpp)
target_link_libraries(libportscan PUBLIC cactus)

add_catch(test_portscan test.cpp)
target_link_libraries(test_portscan PRIVATE libportscan)

add_shad_executable(run_portscan main.cpp)
target_link_libraries(run_portscan PRIVATE libportscan)
