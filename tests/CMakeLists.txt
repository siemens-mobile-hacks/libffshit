find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(ffshit_tests
    fullflash_fixture.cpp
    hash_test.cpp
    write_test.cpp)

target_include_directories(ffshit_tests PRIVATE ${PROJECT_SOURCE_DIR}/inc)
target_link_libraries(ffshit_tests PRIVATE ${PROJECT_NAME} fmt::fmt GTest::gtest_main)

# The fullflashes are not part of the repository: point FFSHIT_TEST_FULLFLASHES at a directory
# holding them, or put them (or a symlink to them) at tests/fullflashes. Without them the write
# tests are skipped.
target_compile_definitions(ffshit_tests PRIVATE FFSHIT_TEST_FULLFLASHES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fullflashes")

gtest_discover_tests(ffshit_tests)
