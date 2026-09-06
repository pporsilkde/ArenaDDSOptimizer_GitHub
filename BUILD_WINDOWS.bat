@echo off
setlocal
if "%QT_ROOT%"=="" (
  echo Set QT_ROOT first, for example:
  echo   set QT_ROOT=C:\Qt\6.8.0\msvc2022_64
  exit /b 1
)
cmake -S . -B build -DCMAKE_PREFIX_PATH="%QT_ROOT%"
if errorlevel 1 exit /b %errorlevel%
cmake --build build --config Release
endlocal
