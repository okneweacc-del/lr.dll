@echo off
setlocal
cd /d "%~dp0"
set "SRC=build\TKVocalStudio_artefacts\Release\VST3\TK VOCAL STUDIO.vst3"
set "DST=%CommonProgramFiles%\VST3\TK VOCAL STUDIO.vst3"

if not exist "%SRC%" (echo [LOI] Chua co file VST3. Hay chay build.bat truoc. & exit /b 1)

net session >nul 2>&1
if errorlevel 1 (echo [LOI] Hay click phai install.bat va chon "Run as administrator". & exit /b 1)

echo Cai dat vao: %DST%
if exist "%DST%" rmdir /s /q "%DST%"
robocopy "%SRC%" "%DST%" /E /NFL /NDL /NJH /NJS >nul
if errorlevel 8 (echo [LOI] Copy that bai. Hay dong FL Studio roi thu lai. & exit /b 1)

echo Da cai xong. Trong FL Studio: Options ^> Manage plugins ^> Find installed plugins (Verify plugins) ^> TK VOCAL STUDIO.
endlocal
