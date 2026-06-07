# Copyright (c) 2026 Cryolite
# SPDX-License-Identifier: MIT
# This file is part of https://github.com/Cryolite/fuhan

cmake_minimum_required(VERSION 3.25.0)
project(FuHan)

option(FUHAN_BUILD_TESTING "Build FuHan tests" OFF)

message("CMAKE_C_COMPILER=${CMAKE_C_COMPILER}")
message("CMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}")

set(CMAKE_CXX_STANDARD 23)

if (CMAKE_BUILD_TYPE STREQUAL "")
  set(CMAKE_BUILD_TYPE Release)
endif()

message("CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")

if (("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU") OR ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"))
  add_compile_options(-Wall -Wextra -Werror)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  if (("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU") OR ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"))
    add_compile_definitions(_GLIBCXX_DEBUG _GLIBCXX_DEBUG_PEDANTIC)
  endif()
endif()

if (FUHAN_WITH_COVERAGE)
  add_compile_definitions(FUHAN_WITH_COVERAGE)
  add_compile_options("-coverage")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -coverage")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -coverage")
endif()
message("FUHAN_WITH_COVERAGE=${FUHAN_WITH_COVERAGE}")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  if (("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU") OR ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"))
    set(FUHAN_WITH_ASAN ON)
  endif()
endif()
if (FUHAN_WITH_ASAN)
  add_compile_options(-fsanitize=address)
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
endif()
message("FUHAN_WITH_ASAN=${FUHAN_WITH_ASAN}")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  if (("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU") OR ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"))
    set(FUHAN_WITH_UBSAN ON)
  endif()
endif()
if (FUHAN_WITH_UBSAN)
  add_compile_options(-fsanitize=undefined)
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=undefined")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=undefined")
endif()
message("FUHAN_WITH_UBSAN=${FUHAN_WITH_UBSAN}")

if (FUHAN_WITH_TSAN)
  add_compile_options(-fsanitize=thread)
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=thread")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=thread")
endif()
message("FUHAN_WITH_TSAN=${FUHAN_WITH_TSAN}")

if (CMAKE_BUILD_TYPE STREQUAL "Release")
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

message("CMAKE_SHARED_LINKER_FLAGS=${CMAKE_SHARED_LINKER_FLAGS}")
message("CMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS}")

add_library(fuhan INTERFACE)
target_sources(fuhan
  INTERFACE
  FILE_SET fuhan_headers
  TYPE HEADERS
  FILES
    fuhan/types.hpp
    fuhan/internal.hpp
    fuhan/standard/internal.hpp
    fuhan/standard/decomposition.hpp
    fuhan/standard/fuhan.hpp
    fuhan/seven_pairs/fuhan.hpp
    fuhan/thirteen_orphans/fuhan.hpp
    fuhan/fuhan.hpp)
target_compile_features(fuhan INTERFACE cxx_std_23)
install(TARGETS fuhan FILE_SET fuhan_headers)

if (FUHAN_BUILD_TESTING)
  enable_testing()
  add_subdirectory(test)
endif()
