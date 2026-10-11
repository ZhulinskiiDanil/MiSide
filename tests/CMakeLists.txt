cmake_minimum_required(VERSION 3.21)
project(miside-menu-tests CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Mita's playback decisions alone: no Geode, no cocos
add_executable(mita-plan-test mita_plan_test.cpp)
target_include_directories(mita-plan-test PRIVATE ../src/ui)

enable_testing()
add_test(NAME mita-plan COMMAND mita-plan-test)
