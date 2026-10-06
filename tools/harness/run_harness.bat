@echo off
rem Offline regression harness (control flow only, no game). Based on Astra's v0.5 audit harness.
rem Copies the CURRENT mod source next to a mock of the Script Hook SDK, builds, runs. Exit code 0 = every scenario FIXED.
rem The mock is made from YOUR copy of the Script Hook RDR2 SDK (which may not be redistributed): the same headers with
rem IMPORT emptied, so the mod links against the harness's fake natives instead of ScriptHookRDR2.dll.
setlocal
cd /d "%~dp0"
if not defined SDK if exist "%~dp0..\..\sdk\inc\main.h" set "SDK=%~dp0..\..\sdk"
if not defined SDK set "SDK=%~dp0..\..\..\downloads\ScriptHookRDR2_SDK"
if not exist "%SDK%\inc\main.h" (echo Script Hook RDR2 SDK not found at "%SDK%" - see README "Building from source" & exit /b 1)
if exist work rmdir /s /q work
mkdir work\NadoTest\src
mkdir work\downloads\ScriptHookRDR2_SDK\inc
xcopy /q /y ..\..\src\*.* work\NadoTest\src\ >nul
xcopy /q /y "%SDK%\inc\*.*" work\downloads\ScriptHookRDR2_SDK\inc\ >nul
powershell -NoProfile -Command "$p='work\downloads\ScriptHookRDR2_SDK\inc\main.h'; (Get-Content $p) -replace '#define IMPORT __declspec\(dllimport\)','#define IMPORT' | Set-Content $p"
copy /y harness.cpp work\ >nul
copy /y hashes.h work\ >nul
rem Visual Studio (2019 or newer, "Desktop development with C++"): found with vswhere, or set VCVARS yourself.
if not defined VCVARS if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VCVARS=%%i\VC\Auxiliary\Build\vcvars64.bat"
if not defined VCVARS set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
call "%VCVARS%" >nul 2>&1
cd work
cl /nologo /Od /MT /EHsc /std:c++17 /W3 /DNOMINMAX /D_CRT_SECURE_NO_WARNINGS /I downloads\ScriptHookRDR2_SDK\inc harness.cpp /Fe:harness.exe /link user32.lib >build.log 2>&1 || (type build.log & exit /b 1)
"%~dp0work\harness.exe"
