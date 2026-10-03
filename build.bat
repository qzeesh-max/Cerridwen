@echo off
setlocal

echo Building Cerridwen Framework...

if not exist "build" mkdir build
cd build

cmake -G "MinGW Makefiles" ..
if %errorlevel% neq 0 exit /b %errorlevel%

cmake --build . -j4
if %errorlevel% neq 0 exit /b %errorlevel%

echo Running tests...
ctest --output-on-failure
if %errorlevel% neq 0 exit /b %errorlevel%

endlocal
