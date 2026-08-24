@echo off

REM --setup mode: configure MSVC env in CALLER's environment.
REM IMPORTANT: must run BEFORE setlocal, otherwise vcvarsall.bat's
REM INCLUDE/LIB/PATH would be discarded when this script exits.
if "%~1"=="--setup" goto :msvc_setup

setlocal enabledelayedexpansion

REM ============================================
REM init_build.bat  -  KForge Build Generator
REM
REM   1. Setup MSVC environment
REM   2. Precompile base/*.cpp -> base/obj/*.obj -> base/KF.lib
REM   3. Scan study/ + test/ for directories containing .cpp files
REM   4. Generate build.bat in each
REM
REM   No args needed. Run from project root.
REM   --setup : MSVC env setup only (called by generated build.bat)
REM ============================================

set "ROOT=%~dp0"
set "ROOT=%ROOT:~0,-1%"
set "BASE=%ROOT%\base"
set "BASE_OBJ=%BASE%\obj"
set "KFLIB=%BASE%\KF.lib"

echo ============================================
echo   KForge - Init Build
echo ============================================
echo.

REM [1/4] MSVC setup
echo [1/4] MSVC environment setup ...
call :msvc_setup
if errorlevel 1 goto :err
echo       Done.
echo.

REM [2/4] Precompile base/KF.lib
echo [2/4] Precompiling base\obj + KF.lib ...
call :precompile_base
if errorlevel 1 goto :err
echo       Done.
echo.

REM [3/4] Clean old scripts
echo [3/4] Cleaning old build.bat ...
for /r "%ROOT%\study" %%b in (build.bat) do if exist "%%b" del "%%b"
for /r "%ROOT%\test"  %%b in (build.bat) do if exist "%%b" del "%%b"
echo       Done.
echo.

REM [4/4] Generate scripts
echo [4/4] Generating build.bat ...
call :gen_scripts "%ROOT%\study"
call :gen_scripts "%ROOT%\test"
echo       Done.
echo.

echo ============================================
echo   All build scripts generated.
echo   Run build.bat in any study\ subdirectory.
echo ============================================
pause
goto :end

REM ========== :msvc_setup ==========
:msvc_setup
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo [ERROR] vswhere.exe not found - install VS 2022
    goto :err
)
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -property installationPath`) do set "VSROOT=%%i"
if not defined VSROOT (
    echo [ERROR] VS installation not found
    goto :err
)
call "%VSROOT%\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>nul
if errorlevel 1 (
    echo [ERROR] vcvarsall.bat failed
    goto :err
)
set "SDKROOT=%ProgramFiles(x86)%\Windows Kits\10"
if not exist "%SDKROOT%\Include" set "SDKROOT=%ProgramFiles%\Windows Kits\10"
for /f "usebackq delims=" %%s in (`dir "%SDKROOT%\Include" /b /ad /o-n 2^>nul`) do (
    set "SDKVER=%%s"
    goto :sdkdone
)
:sdkdone
if defined SDKVER (
    if exist "%SDKROOT%\Include\%SDKVER%\ucrt" (
        set "INCLUDE=%SDKROOT%\Include\%SDKVER%\ucrt;%SDKROOT%\Include\%SDKVER%\um;%SDKROOT%\Include\%SDKVER%\shared;%INCLUDE%"
        set "LIB=%SDKROOT%\Lib\%SDKVER%\ucrt\x64;%SDKROOT%\Lib\%SDKVER%\um\x64;%LIB%"
    )
)
exit /b 0

REM ========== :precompile_base ==========
:precompile_base
set "PCFLAGS=/O2 /std:c++17 /utf-8 /EHsc /MD /I%ROOT%"
if not exist "%BASE_OBJ%" mkdir "%BASE_OBJ%"

set "recompiled=0"
for %%f in ("%BASE%\*.cpp") do (
    set "src=%%f"
    set "obj=%BASE_OBJ%\%%~nf.obj"
    set "need=1"
    if exist "!obj!" (
        for %%a in ("!src!") do for %%b in ("!obj!") do (
            if "%%~ta" LEQ "%%~tb" set "need=0"
        )
    )
    if "!need!"=="1" (
        echo   [COMPILE] %%~nxf
        cl /c !PCFLAGS! "%%f" /Fo:"!obj!"
        if errorlevel 1 (
            echo [ERROR] Failed to compile: %%~nxf
            exit /b 1
        )
        set "recompiled=1"
    ) else (
        echo   [SKIP] %%~nxf
    )
)

if "!recompiled!"=="1" goto :build_lib
if not exist "%KFLIB%" goto :build_lib
echo   [SKIP] KF.lib up-to-date
exit /b 0

:build_lib
echo   [LIB] Creating KF.lib ...
set "LIBOBJS="
for %%f in ("%BASE_OBJ%\*.obj") do set "LIBOBJS=!LIBOBJS! %%f"
lib /out:"%KFLIB%" !LIBOBJS! >nul 2>nul
if errorlevel 1 (
    echo [ERROR] Failed to create KF.lib
    exit /b 1
)
echo   [LIB] KF.lib created.
exit /b 0

REM ========== :gen_scripts - recursive ==========
:gen_scripts
set "dir=%~1"
set "has=0"
for %%f in ("%dir%\*.cpp") do set "has=1"
if "!has!"=="1" (
    call :write_build_bat  "%dir%"
    echo   [GEN] %dir%\build.bat
)
for /d %%d in ("%dir%\*") do call :gen_scripts "%%d"
exit /b 0

REM ========== :write_build_bat ==========
REM   %1 = target directory
:write_build_bat
set "dir=%~1"
set "rel=%dir%"
set "rel=!rel:%ROOT%\=!"
rem Compute relative path from target dir to root
set "to_root="
:rel_loop
if "!rel!"=="" goto :rel_end
set "to_root=!to_root!..\"
set "part=!rel:*\=!"
if "!part!"=="!rel!" (set "rel=") else (set "rel=!part!")
goto :rel_loop
:rel_end
set "out=%dir%\build.bat"
setlocal disabledelayedexpansion

> "%out%" echo @echo off
>>"%out%" echo setlocal enabledelayedexpansion
>>"%out%" echo.
>>"%out%" echo REM Auto-generated by init_build.bat - Selective build
>>"%out%" echo set "DIR=%%~dp0"
>>"%out%" echo set "DIR=!DIR:~0,-1!"
>>"%out%" echo set "KFLIB=%%~dp0%to_root%base\KF.lib"
>>"%out%" echo set "CXXFLAGS=/O2 /std:c++17 /utf-8 /EHsc /MD /I%%~dp0%to_root%"
>>"%out%" echo set "LINKFLAGS=/OPT:REF /OPT:ICF /SUBSYSTEM:CONSOLE"
>>"%out%" echo.
>>"%out%" echo REM Setup MSVC environment if not already set
>>"%out%" echo if not defined INCLUDE call "%%~dp0%to_root%init_build.bat" --setup
>>"%out%" echo.
>>"%out%" echo set "filecount=0"
>>"%out%" echo for %%%%f in ("!DIR!\*.cpp") do (
>>"%out%" echo     set /a filecount+=1
>>"%out%" echo     set "file_!filecount!=%%%%~nxf"
>>"%out%" echo     set "filepath_!filecount!=%%%%f"
>>"%out%" echo )
>>"%out%" echo.
>>"%out%" echo if "!filecount!"=="0" (
>>"%out%" echo     echo No .cpp files found.
>>"%out%" echo     pause
>>"%out%" echo     exit /b 0
>>"%out%" echo )
>>"%out%" echo.
>>"%out%" echo echo +----------------------------------------+
>>"%out%" echo echo   KForge Build  -  %rel%
>>"%out%" echo echo +----------------------------------------+
>>"%out%" echo echo.
>>"%out%" echo echo   Files in this directory:
>>"%out%" echo for /l %%%%i in (1,1,!filecount!) do echo     %%%%i. !file_%%%%i!
>>"%out%" echo echo.
>>"%out%" echo set "choice="
>>"%out%" echo set /p "choice=  Select numbers (e.g. 1 3) or Enter for all: "
>>"%out%" echo.
>>"%out%" echo if not exist "!DIR!\Release" mkdir "!DIR!\Release"
>>"%out%" echo.
>>"%out%" echo set "compileall=1"
>>"%out%" echo if defined choice (
>>"%out%" echo     set "sel=!choice:,= !"
>>"%out%" echo     set "seltrimmed=!sel: =!"
>>"%out%" echo     if not "!seltrimmed!"=="" set "compileall=0"
>>"%out%" echo )
>>"%out%" echo.
>>"%out%" echo if "!compileall!"=="1" (
>>"%out%" echo     for /l %%%%i in (1,1,!filecount!) do (
>>"%out%" echo         call :compile_one "!filepath_%%%%i!" "!file_%%%%i:~0,-4!"
>>"%out%" echo         if errorlevel 1 goto :build_err
>>"%out%" echo     )
>>"%out%" echo ) else (
>>"%out%" echo     for %%%%n in (!sel!) do (
>>"%out%" echo         if defined filepath_%%%%n (
>>"%out%" echo             call :compile_one "!filepath_%%%%n!" "!file_%%%%n:~0,-4!"
>>"%out%" echo             if errorlevel 1 goto :build_err
>>"%out%" echo         ) else (
>>"%out%" echo             echo   [WARN] Invalid: %%%%n
>>"%out%" echo         )
>>"%out%" echo     )
>>"%out%" echo )
>>"%out%" echo.
>>"%out%" echo echo.
>>"%out%" echo echo   [DONE] Build complete. Output: !DIR!\Release\
>>"%out%" echo pause
>>"%out%" echo goto :build_end
>>"%out%" echo.
>>"%out%" echo :compile_one
>>"%out%" echo set "cf=%%~1"
>>"%out%" echo set "cn=%%~2"
>>"%out%" echo echo   [BUILD] !cn!.cpp
>>"%out%" echo cl /c !CXXFLAGS! "!cf!" /Fo:"!DIR!\Release\!cn!.obj"
>>"%out%" echo if errorlevel 1 (
>>"%out%" echo     echo   [FAIL] !cn!.cpp - compile error
>>"%out%" echo     exit /b 1
>>"%out%" echo )
>>"%out%" echo link !LINKFLAGS! "!KFLIB!" "!DIR!\Release\!cn!.obj" /out:"!DIR!\Release\!cn!.exe"
>>"%out%" echo if errorlevel 1 (
>>"%out%" echo     echo   [FAIL] !cn!.cpp - link error
>>"%out%" echo     exit /b 1
>>"%out%" echo )
>>"%out%" echo del "!DIR!\Release\!cn!.obj"
>>"%out%" echo echo   [OK] !cn!.cpp
>>"%out%" echo exit /b 0
>>"%out%" echo.
>>"%out%" echo :build_err
>>"%out%" echo echo   [ERROR] Build failed.
>>"%out%" echo pause
>>"%out%" echo exit /b 1
>>"%out%" echo.
>>"%out%" echo :build_end
>>"%out%" echo endlocal
>>"%out%" echo exit /b 0
endlocal
exit /b 0

REM ========== error / end ==========
:err
echo [ERROR] Task failed.
pause
exit /b 1

:end
endlocal
exit /b 0