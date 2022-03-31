cmake_minimum_required(VERSION 3.0.0)
project(jlang VERSION 0.1.0)

include(CTest)
enable_testing()

find_program(CLANG_FORMAT "clang-format")
set(CMAKE_CXX_COMPILER clang++)
set(TARGET_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB TARGET_CXX_SOURCE_FILES ${TARGET_SOURCE_DIR}/src/*.cpp)
file(GLOB TARGET_CXX_INCLUDE_FILES ${TARGET_SOURCE_DIR}/include/*.hpp)
set(TARGET_CLANG_FORMAT_FILES ${TARGET_CXX_SOURCE_FILES} ${TARGET_CXX_INCLUDE_FILES} main.cpp)

#set(CMAKE_CXX_CLANG_TIDY
#    clang-tidy;
#    -checks=*;)

add_executable(jlang main.cpp ${TARGET_CXX_SOURCE_FILES})
set_property(TARGET jlang PROPERTY CXX_STANDARD 20)
target_include_directories(jlang PRIVATE ${TARGET_SOURCE_DIR}/include ${TARGET_SOURCE_DIR}/src)

if(CLANG_FORMAT)
    add_custom_target(
        clang-format
        COMMAND /usr/bin/clang-format
        -i
        ${TARGET_CLANG_FORMAT_FILES}   
    )

else()
    message(FATAL_ERROR "clang-format not found")
endif()


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
