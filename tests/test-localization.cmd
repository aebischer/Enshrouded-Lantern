@echo off
setlocal
cd /d "%~dp0.."
call tools\vcvars.cmd
if errorlevel 1 exit /b 1
if not exist build mkdir build
powershell -NoProfile -ExecutionPolicy Bypass -File sdk\tools\compile-translations.ps1 -Catalog assets\localization\texts.json -Output src\lantern_texts.h -Symbol LanternTexts
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /utf-8 /EHsc /O2 /MT /W4 /WX /DNOMINMAX /DWIN32_LEAN_AND_MEAN /Ibuild /Isdk\include /Ithird_party\minhook\include tests\localization.cpp build\buffer.obj build\hook.obj build\trampoline.obj build\hde64.obj build\lantern_model.res bcrypt.lib /Fo:build\localization.obj /Fe:build\localization-test.exe
if errorlevel 1 exit /b 1
build\localization-test.exe
exit /b %errorlevel%
