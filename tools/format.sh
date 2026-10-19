#!/bin/sh
set -e
echo '[INFO] Formatting...'
find src/ -type f \( -name '*.[hc]' -o -name '*.[hc]pp' \) -print -exec clang-format -i {} +
echo '[INFO] Finished formatting.'

