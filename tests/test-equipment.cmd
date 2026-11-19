@echo off
cd /d "%~dp0.."
call tools\vcvars.cmd
if errorlevel 1 exit /b 1
pushd build
cl /nologo /std:c++17 /EHsc /W4 /WX ..\tests\equipment_runtime.cpp bcrypt.lib /Fe:equipment-runtime-test.exe
if errorlevel 1 exit /b 1
equipment-runtime-test.exe
