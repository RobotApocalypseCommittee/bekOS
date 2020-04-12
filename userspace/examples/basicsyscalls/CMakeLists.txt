add_executable(example_basicsyscalls main.c)
target_link_libraries(example_basicsyscalls bekoslibc "-Wl,-emain")
set_target_properties(example_basicsyscalls PROPERTIES OUTPUT_NAME "exec1")