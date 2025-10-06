@echo off
setlocal enabledelayedexpansion

set "BUILD_TYPE=%~1"
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Release

set "ROOT=%~dp0.."
set "BUILD_DIR=%ROOT%\build\msvc"

echo Configuring STRAP (%BUILD_TYPE%)...
cmake -S "%ROOT%" -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -A x64 -DSTRAP_BUILD_TESTS=ON -DSTRAP_BUILD_BENCHMARKS=OFF -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
if errorlevel 1 (
    echo Failed to configure STRAP project.
    exit /b 1
)

echo Building STRAP...
cmake --build "%BUILD_DIR%" --config %BUILD_TYPE%
if errorlevel 1 (
    echo Build failed.
    exit /b 1
)

echo Running tests...
ctest --test-dir "%BUILD_DIR%" --config %BUILD_TYPE% --output-on-failure
exit /b %errorlevel%
