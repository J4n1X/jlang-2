cmake_minimum_required(VERSION 3.0.0)
project(jlang VERSION 0.1.0)

include(CTest)
enable_testing()

set(TARGET_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(jlang main.cpp src/Tokenizer.cpp src/JlangObjects.cpp src/Statements.cpp)
set_property(TARGET jlang PROPERTY CXX_STANDARD 20)
target_include_directories(jlang PRIVATE ${TARGET_SOURCE_DIR}/include ${TARGET_SOURCE_DIR}/src)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
