# Conversion tests: run the HexStartup conversion code on the images in sample/ and decode the lumps it writes.
# The MT2D UI calls are stubbed (mt2d_stubs.cpp), so the tests need no terminal or window.
#
#   cmake --build build && ctest --test-dir build --output-on-failure

add_executable(test_conversion
    test_conversion.cpp
    mt2d_stubs.cpp
    ${PROJECT_SOURCE_DIR}/Image.cpp
    ${PROJECT_SOURCE_DIR}/Hexen.cpp
    ${PROJECT_SOURCE_DIR}/PPM.cpp
    ${PROJECT_SOURCE_DIR}/_PNG.cpp
)
target_include_directories(test_conversion PRIVATE ${PROJECT_SOURCE_DIR})
# mt2d is linked for its include paths and backend defines; the stubs provide every MT2D symbol the code uses.
target_link_libraries(test_conversion PRIVATE mt2d::mt2d ${HEXSTARTUP_PNG_LIBS})
set_target_properties(test_conversion PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)
if(MSVC)
    target_compile_definitions(test_conversion PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(test_conversion PRIVATE -Wno-write-strings)
endif()

set(TEST_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/output")
file(MAKE_DIRECTORY "${TEST_OUTPUT_DIR}")
add_test(NAME sample_conversion
    COMMAND test_conversion "${PROJECT_SOURCE_DIR}/sample" "${TEST_OUTPUT_DIR}")
