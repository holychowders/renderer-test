@echo off
Echo Formatting...
For %%d in (src) do (
    Echo Formatting in "%%~d"
    PushD "%%~d" 
    For /r %%f In (*.c *.cpp *.h *.hpp) Do (
        Echo Formatting "%%f"
        clang-format -i "%%f"
    )
    PopD
)
