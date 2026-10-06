@echo off
rem Builds bin\NadoTest.asi with the VS 2019 Build Tools (x64). Run from any directory.
setlocal
set ROOT=%~dp0
rem The Script Hook RDR2 SDK: set SDK yourself, or put it in sdk\ next to this file (sdk\inc\main.h, sdk\lib\ScriptHookRDR2.lib).
if not defined SDK if exist "%ROOT%sdk\inc\main.h" set SDK=%ROOT%sdk
if not defined SDK set SDK=%ROOT%..\downloads\ScriptHookRDR2_SDK
if not exist "%SDK%\inc\main.h" (echo Script Hook RDR2 SDK not found at "%SDK%" - see README "Building from source" & exit /b 1)
rem Visual Studio (2019 or newer, "Desktop development with C++"): found with vswhere, or set VCVARS yourself.
if not defined VCVARS if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VCVARS=%%i\VC\Auxiliary\Build\vcvars64.bat"
if not defined VCVARS set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
call "%VCVARS%" >nul || exit /b 1
if not exist "%ROOT%bin" mkdir "%ROOT%bin"
if not exist "%ROOT%obj" mkdir "%ROOT%obj"
cl /nologo /O2 /MT /EHsc /std:c++17 /W3 /DNDEBUG /DNOMINMAX /D_CRT_SECURE_NO_WARNINGS /I "%SDK%\inc" /Fo"%ROOT%obj\\" ^
  "%ROOT%src\main.cpp" "%ROOT%src\common.cpp" "%ROOT%src\input.cpp" "%ROOT%src\pad.cpp" "%ROOT%src\tornado.cpp" "%ROOT%src\script.cpp" ^
  "%ROOT%src\ui.cpp" "%ROOT%src\roar.cpp" ^
  /LD /Fe"%ROOT%bin\NadoTest.asi" /link /NOLOGO "%SDK%\lib\ScriptHookRDR2.lib" user32.lib || exit /b 1
copy /Y "%ROOT%NadoTest.ini" "%ROOT%bin\NadoTest.ini" >nul
echo BUILD OK
