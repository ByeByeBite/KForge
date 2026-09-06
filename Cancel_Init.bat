@echo off
setlocal

REM ============================================
REM Cancel_Init.bat
REM   Clean up ALL build artifacts:
REM     - all Release\ folders tree-wide
REM     - build.bat in study/ and test/
REM     - build trees: build\ build-msvc\ build20\
REM     - stray \!DIR!\ + _cmake_test\
REM     - stale legacy KF artifacts
REM   Note: .analysis\ is intentionally kept.
REM ============================================

echo ============================================
echo   KForge - Cancel Init (Cleanup)
echo ============================================
echo.

set "ROOT=%~dp0"
set "ROOT=%ROOT:~0,-1%"

echo [1/6] Deleting all Release\ folders tree-wide ...
for /d /r "%ROOT%" %%r in (Release) do rd /s /q "%%r" 2>nul
echo       Done.

echo [2/6] Deleting build.bat in study\ + test\ ...
for /r "%ROOT%\study" %%b in (build.bat) do if exist "%%b" del "%%b"
if exist "%ROOT%\test\build.bat" del "%ROOT%\test\build.bat"
echo       Done.

echo [3/6] Deleting build trees: build\ build-msvc\ build20\ build-ide\ build-null\ ...
if exist "%ROOT%\build" rd /s /q "%ROOT%\build"
if exist "%ROOT%\build-msvc" rd /s /q "%ROOT%\build-msvc"
if exist "%ROOT%\build20" rd /s /q "%ROOT%\build20"
if exist "%ROOT%\build-ide" rd /s /q "%ROOT%\build-ide"
if exist "%ROOT%\build-null" rd /s /q "%ROOT%\build-null"
echo       Done.

echo [4/6] Deleting stray \!DIR!\ + _cmake_test\ ...
if exist "%ROOT%\!DIR!" rd /s /q "%ROOT%\!DIR!"
if exist "%ROOT%\_cmake_test" rd /s /q "%ROOT%\_cmake_test"
echo       Done.

echo [5/6] Deleting stale module artifacts ...
if exist "%ROOT%\modules\cpp\KF.lib" del "%ROOT%\modules\cpp\KF.lib"
if exist "%ROOT%\modules\cpp\obj" rd /s /q "%ROOT%\modules\cpp\obj"
echo       Done.

echo [6/6] Deleting <root>\Release\ (legacy top-level exes) ...
if exist "%ROOT%\Release" rd /s /q "%ROOT%\Release"
echo       Done.

echo.
echo All build artifacts cleaned up.
pause
endlocal
exit /b 0