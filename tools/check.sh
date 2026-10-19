#!/bin/sh
set -e
echo '[INFO] Linting...'
find src/ -type f \( -name '*.[hc]' -o -name '*.[hc]pp' \) -print -exec clang-tidy {} +
echo '[INFO] Finished linting.'
