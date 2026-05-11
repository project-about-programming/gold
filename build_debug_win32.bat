@echo off
setlocal
cd /d "%~dp0"
where msbuild >nul 2>nul
if errorlevel 1 (
  echo [ERROR] MSBuild not found. Open this project in Visual Studio or use Developer Command Prompt.
  pause
  exit /b 1
)
msbuild SalesFlowCLR.sln /p:Configuration=Debug /p:Platform=Win32
if errorlevel 1 (
  echo [ERROR] Build failed.
  pause
  exit /b 1
)
start "" "bin\Debug\Win32\SalesFlowCLR.exe"
