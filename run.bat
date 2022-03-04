@echo off

set cmake_dir=%cd%\tools\cmake-3.23.0-rc2-windows-x86_64\bin
set ninja_dir=%cd%\tools\ninja-win
set source_dir=%cd%
set destination_dir=%cd%\debug

rem echo %cmake_dir%
rem echo %ninja_dir%
rem echo %source_dir%
rem echo %destination_dir%

rd /s/q %destination_dir%
%cmake_dir%\cmake.exe "-DCMAKE_BUILD_TYPE=Debug" "-DCMAKE_MAKE_PROGRAM=%ninja_dir%\ninja.exe" -G Ninja -S %source_dir% -B %destination_dir%
%cmake_dir%\cmake.exe --build %destination_dir% --target homework
xcopy %source_dir%\*.csv %destination_dir%
cd %destination_dir%
if exist homework.exe call homework.exe

pause
