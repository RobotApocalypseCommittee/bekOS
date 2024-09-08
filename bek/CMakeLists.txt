add_library(bek
        src/string.cpp
        src/utility.cpp
        src/time.cpp
        src/memory.cpp
        src/format.cpp)

if (BEKOS_ARCHITECTURE STREQUAL "a64")
    target_sources(bek PRIVATE src/a64/memory.S)
endif ()

target_include_directories(bek PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_features(bek PRIVATE cxx_std_20)
#set_source_files_properties(src/memory.cpp PROPERTIES COMPILE_FLAGS )

target_compile_options(bek PUBLIC -static -nostdlib -nostdinc++ -fdata-sections -ffunction-sections PRIVATE -fno-exceptions -fno-rtti)