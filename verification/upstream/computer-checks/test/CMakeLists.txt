enable_testing()
include(GoogleTest)

# test
function(add_gtest_executable test_name)
    add_executable(${test_name} ${test_name}.cpp)
    target_link_libraries(${test_name} PRIVATE
        GTest::gtest_main
        project_deps
        combine_lib
    )
    gtest_discover_tests(${test_name})
endfunction()

add_gtest_executable(cartwheel_test)
add_gtest_executable(configuration_test)
add_gtest_executable(degree_test)
add_gtest_executable(pseudo_configuration_test)
add_gtest_executable(pseudo_triangulation_test)
add_gtest_executable(rule_test)
