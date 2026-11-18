@echo off
setlocal
cd /d "%~dp0"
call tools\vcvars.cmd
if errorlevel 1 exit /b 1
if not exist build mkdir build
if not exist dist\Mods\lantern mkdir dist\Mods\lantern
powershell -NoProfile -ExecutionPolicy Bypass -File tools\embed-icon.ps1
if errorlevel 1 exit /b 1
pushd build
rc /nologo /fo lantern_model.res ..\src\lantern_model.rc
if errorlevel 1 exit /b 1
cl /nologo /O2 /MT /c /I..\third_party\minhook\include ..\third_party\minhook\src\buffer.c ..\third_party\minhook\src\hook.c ..\third_party\minhook\src\trampoline.c ..\third_party\minhook\src\hde\hde64.c
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /utf-8 /EHsc /O2 /MT /W4 /WX /LD /DNOMINMAX /DWIN32_LEAN_AND_MEAN /I. /I..\sdk\include /I..\third_party\minhook\include ..\src\lantern.cpp ..\src\native_content.cpp buffer.obj hook.obj trampoline.obj hde64.obj lantern_model.res bcrypt.lib /Fe:lantern-map.dll /link /IMPLIB:lantern-map.lib /MAP:lantern.map
if errorlevel 1 exit /b 1
popd
rem Diagnostic link only
exit /b %errorlevel%

