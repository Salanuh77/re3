add_executable(physics_tests fixed_step_tests.cpp)
target_include_directories(physics_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../src/core")
set_target_properties(physics_tests PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON)
add_test(NAME physics_unit COMMAND physics_tests)

add_executable(camera_transform_tests camera_transform_tests.cpp ../src/math/Quaternion.cpp)
target_include_directories(camera_transform_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/support"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/math"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/core")
set_target_properties(camera_transform_tests PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON)
add_test(NAME camera_transforms COMMAND camera_transform_tests)

option(RE3_BDD_TESTS "Run Gherkin tests using Python behave" OFF)
if(RE3_BDD_TESTS)
    set(Python3_FIND_STRATEGY LOCATION)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    execute_process(COMMAND "${Python3_EXECUTABLE}" -c "import behave"
        RESULT_VARIABLE RE3_BEHAVE_IMPORT_RESULT OUTPUT_QUIET ERROR_QUIET)
    if(NOT RE3_BEHAVE_IMPORT_RESULT EQUAL 0)
        message(FATAL_ERROR "Install tests/requirements.txt using ${Python3_EXECUTABLE}, or set Python3_EXECUTABLE to your Behave interpreter")
    endif()
    add_test(NAME physics_gherkin COMMAND "${Python3_EXECUTABLE}" -m behave
        "${CMAKE_CURRENT_SOURCE_DIR}/features" --no-capture
        -D "physics_probe=$<TARGET_FILE:physics_tests>"
        -D "camera_probe=$<TARGET_FILE:camera_transform_tests>")
endif()
