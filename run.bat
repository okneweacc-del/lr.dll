@echo off
cd /d "%~dp0"
set "EXE=build\TKVocalStudio_artefacts\Release\Standalone\TK VOCAL STUDIO.exe"
if not exist "%EXE%" (echo Chua build. Chay build.bat truoc. & exit /b 1)
start "" "%EXE%"
