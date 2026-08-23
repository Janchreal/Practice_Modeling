@echo off

set QT_DIR=d:\Qt\6.8.3\msvc2022_64
set SRC_DIR=%cd%
set BUILD_DIR=%cd%\build

if not exist %QT_DIR% exit
if not exist %SRC_DIR% exit
if not exist %BUILD_DIR% md %BUILD_DIR%

cd build

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

%QT_DIR%\bin\qmake.exe %SRC_DIR%\Modeling.pro -spec win32-msvc  "CONFIG+=release"
if exist %BUILD_DIR%\release\Modeling.exe del %BUILD_DIR%\release\Modeling.exe
nmake Release
if not exist %BUILD_DIR%\release\Qt5Core.dll (
  %QT_DIR%\bin\windeployqt.exe %BUILD_DIR%\release\Modeling.exe
)