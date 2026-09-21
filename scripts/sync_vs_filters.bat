@echo off
setlocal

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0sync_vcxproj_filters.ps1" %*
exit /b %ERRORLEVEL%
