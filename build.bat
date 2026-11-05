@echo off
setlocal EnableDelayedExpansion
cd /d "%~dp0"
echo ============================================
echo  TK VOCAL STUDIO - build (Windows 64-bit)
echo ============================================

where cmake >nul 2>nul || (echo [LOI] Chua cai CMake. Tai tai https://cmake.org/download/ roi chay lai. & exit /b 1)
where git >nul 2>nul   || (echo [LOI] Chua cai Git. Tai tai https://git-scm.com/download/win roi chay lai. & exit /b 1)

REM Can Visual Studio 2022 voi workload "Desktop development with C++"
set "GEN=Visual Studio 17 2022"

if not exist "extern\JUCE\CMakeLists.txt" (
    echo Dang tai JUCE 8.0.4 vao extern\JUCE ...
    git clone --depth 1 --branch 8.0.4 https://github.com/juce-framework/JUCE.git extern\JUCE || (echo [LOI] Khong tai duoc JUCE. & exit /b 1)
)

cmake -S . -B build -G "%GEN%" -A x64 || (echo [LOI] cmake configure that bai. Kiem tra da cai Visual Studio 2022 + C++ workload. & exit /b 1)
cmake --build build --config Release --target TKVocalStudio_VST3 TKVocalStudio_Standalone --parallel || (echo [LOI] Build that bai. & exit /b 1)

echo.
echo Build xong:
echo   VST3      : build\TKVocalStudio_artefacts\Release\VST3\TK VOCAL STUDIO.vst3
echo   Standalone: build\TKVocalStudio_artefacts\Release\Standalone\TK VOCAL STUDIO.exe
echo Chay install.bat (Run as administrator) de cai VST3 cho FL Studio.
endlocal
