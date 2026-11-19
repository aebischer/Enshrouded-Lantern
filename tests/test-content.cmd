@echo off
cd /d "%~dp0.."
call tools\vcvars.cmd
if errorlevel 1 exit /b 1
pushd build
cl /nologo /std:c++17 /EHsc /W4 /WX ..\tests\content.cpp /Fe:content-test.exe
if errorlevel 1 exit /b 1
content-test.exe
