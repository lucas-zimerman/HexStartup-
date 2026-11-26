# Conversion tests: run the HexStartup conversion code on the images in sample/ and decode the lumps it writes.
# The MT2D UI calls are stubbed (mt2d_stubs.cpp), so the tests need no terminal or window.
#
#   cmake --build build && ctest --test-dir build --output-on-failure

function(hexstartup_add_test_executable name)
    add_executable(${name}
        ${ARGN}
        mt2d_stubs.cpp
        ${PROJECT_SOURCE_DIR}/src/Image.cpp
        ${PROJECT_SOURCE_DIR}/src/Hexen.cpp
        ${PROJECT_SOURCE_DIR}/src/PPM.cpp
        ${PROJECT_SOURCE_DIR}/src/_PNG.cpp
        ${PROJECT_SOURCE_DIR}/src/SpriteImage.cpp
    )
    target_include_directories(${name} PRIVATE ${PROJECT_SOURCE_DIR}/src)
    # mt2d is linked for its include paths and backend defines; the stubs provide every MT2D symbol the code uses.
    target_link_libraries(${name} PRIVATE mt2d::mt2d ${HEXSTARTUP_PNG_LIBS})
    set_target_properties(${name} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED ON
    )
    if(MSVC)
        target_compile_definitions(${name} PRIVATE _CRT_SECURE_NO_WARNINGS)
    else()
        target_compile_options(${name} PRIVATE -Wno-write-strings)
    endif()
endfunction()

# Command line mode: the test repeats the steps of src/Source.cpp's main().
hexstartup_add_test_executable(test_conversion test_conversion.cpp)
set(TEST_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/output")
file(MAKE_DIRECTORY "${TEST_OUTPUT_DIR}")
add_test(NAME sample_conversion
    COMMAND test_conversion "${PROJECT_SOURCE_DIR}/sample" "${TEST_OUTPUT_DIR}")

# Interactive mode: runs GenerateHexenStartup itself, which writes its lumps into the current directory.
hexstartup_add_test_executable(test_interactive test_interactive.cpp ${PROJECT_SOURCE_DIR}/src/Interactive.cpp)
set(TEST_INTERACTIVE_DIR "${CMAKE_CURRENT_BINARY_DIR}/output_interactive")
file(MAKE_DIRECTORY "${TEST_INTERACTIVE_DIR}")
add_test(NAME sample_interactive
    COMMAND test_interactive "${PROJECT_SOURCE_DIR}/sample"
    WORKING_DIRECTORY "${TEST_INTERACTIVE_DIR}")
