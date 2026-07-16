@echo off
echo Formatting...
for %%d in (src) do (
    echo Formatting in "%%~d"
    pushd "%%~d" 
    for /r %%f in (*.c *.cpp *.h *.hpp) do (
        echo Formatting "%%f"
        clang-format -i "%%f"
    )
    popd
)
