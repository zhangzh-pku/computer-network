cmake_policy(SET CMP0048 NEW)

project(Lab1-myFTP-test)
cmake_minimum_required(VERSION 3.11)
enable_testing()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
)
set(gtest_force_shared_crt ON)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)
add_executable(ftp_test test.cpp)
target_link_libraries(ftp_test GTest::gtest_main)
gtest_discover_tests(ftp_test)
