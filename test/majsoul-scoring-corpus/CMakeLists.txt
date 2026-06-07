# Copyright (c) 2026 Cryolite
# SPDX-License-Identifier: MIT
# This file is part of https://github.com/Cryolite/fuhan

add_executable(majsoul-scoring-corpus-test-runner
  test-runner.cpp)
target_link_libraries(majsoul-scoring-corpus-test-runner
  PRIVATE fuhan
  PRIVATE Threads::Threads)

add_test(NAME test-majsoul-scoring-corpus
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/test-runner.sh
          $<TARGET_FILE:majsoul-scoring-corpus-test-runner>
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
