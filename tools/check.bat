@echo off
SetLocal EnableDelayedExpansion

For /r src %%f in (*.c *.cpp) Do Set FILES=!FILES! "%%f"
clang-tidy -p .\build\debug\ %FILES%
