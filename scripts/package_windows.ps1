param(
    [string]$BuildDir = "build/release",
    [string]$OutputDir = "package/windows"
)
$ErrorActionPreference = "Stop"
$root = (Resolve-Path "$PSScriptRoot/..").Path
$build = Join-Path $root $BuildDir
$out = Join-Path $root $OutputDir
Remove-Item $out -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $out | Out-Null
$exe = Get-ChildItem -Path $build -Filter ArenaDDSOptimizer.exe -Recurse | Select-Object -First 1
if (-not $exe) { throw "ArenaDDSOptimizer.exe not found in $build" }
Copy-Item $exe.FullName $out
$deploy = Get-Command windeployqt.exe -ErrorAction Stop
& $deploy.Source --release --no-translations --compiler-runtime (Join-Path $out "ArenaDDSOptimizer.exe")
Copy-Item (Join-Path $root "presets") $out -Recurse
Copy-Item (Join-Path $root "README.md") $out
Copy-Item (Join-Path $root "README_RU.md") $out
Copy-Item (Join-Path $root "CHANGELOG.md") $out
if (Test-Path (Join-Path $root "LICENSE")) { Copy-Item (Join-Path $root "LICENSE") $out }
Write-Host "Portable package: $out"
