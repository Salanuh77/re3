add_executable(physics_tests fixed_step_tests.cpp)
target_include_directories(physics_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../src/core")
set_target_properties(physics_tests PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON)
add_test(NAME physics_unit COMMAND physics_tests)

option(RE3_BDD_TESTS "Run Gherkin tests using Python behave" OFF)
if(RE3_BDD_TESTS)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(NAME physics_gherkin COMMAND "${Python3_EXECUTABLE}" -m behave
        "${CMAKE_CURRENT_SOURCE_DIR}/features" --no-capture
        -D "physics_probe=$<TARGET_FILE:physics_tests>")
endif()
