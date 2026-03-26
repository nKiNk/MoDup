@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
rc /fo MoDup.res MoDup.rc
if errorlevel 1 exit /b %ERRORLEVEL%
cl /EHsc /std:c++17 MoDup.cpp MoDup.res User32.lib Advapi32.lib Gdi32.lib Shell32.lib Ole32.lib dwmapi.lib Msimg32.lib Setupapi.lib /Fe:MoDup.exe
echo Exit code: %ERRORLEVEL%
exit /b %ERRORLEVEL%
