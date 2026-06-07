# Copyright (c) 2026 Cryolite
# SPDX-License-Identifier: MIT
# This file is part of https://github.com/Cryolite/fuhan

find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
add_library(fuhan-test-python INTERFACE)
target_link_libraries(fuhan-test-python
  INTERFACE
    Python3::Module
    Python3::Python)

message("Python3_EXECUTABLE=${Python3_EXECUTABLE}")

# Boost.Python is suffixed with the target Python version
# (e.g. boost_python314 for Python 3.14), so derive the component name
# from the Python version discovered for the tests.
set(FUHAN_BOOST_PYTHON_COMPONENT
  "python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}")
# Prefer the Boost package config (BoostConfig.cmake) over the removed
# bundled FindBoost module on CMake >= 3.30.
if (POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()
find_package(Boost REQUIRED COMPONENTS ${FUHAN_BOOST_PYTHON_COMPONENT})

add_library(fuhan-test-boost-python INTERFACE)
target_link_libraries(fuhan-test-boost-python
  INTERFACE
    Boost::${FUHAN_BOOST_PYTHON_COMPONENT}
    fuhan-test-python)

add_executable(test-chiniisou
  run.cpp)
target_link_libraries(test-chiniisou
  PRIVATE fuhan
  PRIVATE fuhan-test-boost-python
  PRIVATE Threads::Threads)

add_test(NAME test-chiniisou
  COMMAND test-chiniisou "${Python3_EXECUTABLE}")
