cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED PASS_DIR)
    message(FATAL_ERROR "PASS_DIR was not provided. Call cmake with -DPASS_DIR=<pass folder>.")
endif()

project(${PASS_DIR})
find_package(LLVM REQUIRED CONFIG)
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)

separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})
include_directories(${LLVM_INCLUDE_DIRS})

add_subdirectory(${PASS_DIR})