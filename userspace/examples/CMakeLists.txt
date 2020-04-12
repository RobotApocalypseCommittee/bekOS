set(examples basicsyscalls)


set(example_targets)
foreach(eg ${examples})
    add_subdirectory(${eg})
    list(APPEND example_targets example_${eg})
endforeach()

set_target_properties(${example_targets} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/example_bin")
add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/fs.img
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/dir2fat32.sh ARGS -f ${CMAKE_BINARY_DIR}/fs.img 8 ${CMAKE_BINARY_DIR}/example_bin
        DEPENDS ${example_targets})
add_custom_target(examples DEPENDS ${CMAKE_BINARY_DIR}/fs.img)
