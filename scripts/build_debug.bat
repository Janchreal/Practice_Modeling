@echo off
setlocal

set "QT_DIR=D:\Qt\6.8.3\msvc2022_64"
set "SRC_DIR=%~dp0.."
set "BUILD_DIR=%SRC_DIR%\build"
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if not exist "%QT_DIR%\bin\qmake.exe" exit /b 1
if not exist "%VSWHERE%" exit /b 1
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS_INSTALL=%%i"
if not defined VS_INSTALL exit /b 1

call "%VS_INSTALL%\VC\Auxiliary\Build\vcvarsall.bat" x64
if errorlevel 1 exit /b 1

pushd "%BUILD_DIR%"
"%QT_DIR%\bin\qmake.exe" "%SRC_DIR%\Practice_Modeling.pro" -spec win32-msvc "CONFIG+=debug" "CONFIG+=console"
if errorlevel 1 (popd & exit /b 1)
nmake
if errorlevel 1 (popd & exit /b 1)

if exist "debug\Practice_Modeling.exe" "%QT_DIR%\bin\windeployqt.exe" --debug --compiler-runtime "debug\Practice_Modeling.exe"
popd
endlocal
