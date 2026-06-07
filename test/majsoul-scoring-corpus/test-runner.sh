#!/usr/bin/env bash

set -euxo pipefail

if [[ $# -ne 1 ]]; then
  echo "Usage: $0 <test-runner>"
  exit 1
fi

TEST_RUNNER="$1"

(
  shopt -s nullglob

  for TEST_FILE in ./*.tsv; do
    cat "$TEST_FILE" | ASAN_OPTIONS=handle_abort=1 "$TEST_RUNNER"
  done

  for TEST_FILE in ./*.tsv.gz; do
    zcat "$TEST_FILE" | ASAN_OPTIONS=handle_abort=1 "$TEST_RUNNER"
  done

  for TEST_FILE in ./*.tsv.bz2; do
    bzcat "$TEST_FILE" | ASAN_OPTIONS=handle_abort=1 "$TEST_RUNNER"
  done

  for TEST_FILE in ./*.tsv.xz; do
    xzcat "$TEST_FILE" | ASAN_OPTIONS=handle_abort=1 "$TEST_RUNNER"
  done
)
