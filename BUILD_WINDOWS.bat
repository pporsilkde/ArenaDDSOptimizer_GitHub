@echo off
setlocal
cd /d "%~dp0"

where git >nul 2>nul || (echo ERROR: git not found & exit /b 1)
where cmake >nul 2>nul || (echo ERROR: cmake not found & exit /b 1)
where pwsh >nul 2>nul || (echo ERROR: PowerShell 7 ^(pwsh^) not found & exit /b 1)

pwsh -NoProfile -ExecutionPolicy Bypass -File scripts\build_directxtex.ps1 -Tag may2026 || exit /b 1
cmake --preset release || exit /b 1
cmake --build --preset release --parallel || exit /b 1
pwsh -NoProfile -ExecutionPolicy Bypass -File scripts\package_windows.ps1 || exit /b 1

echo.
echo Ready: package\windows\ArenaDDSOptimizer.exe
echo Bundled: package\windows\texconv.exe
endlocal
