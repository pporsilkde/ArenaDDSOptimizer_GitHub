param(
    [string]$BuildDir = "build/release",
    [string]$DirectXTexBuildDir = "build/directxtex",
    [string]$DirectXTexSourceDir = "third_party/DirectXTex",
    [string]$OutputDir = "package/windows"
)
$ErrorActionPreference = "Stop"
$root = (Resolve-Path "$PSScriptRoot/..").Path
$build = Join-Path $root $BuildDir
$dxBuild = Join-Path $root $DirectXTexBuildDir
$dxSource = Join-Path $root $DirectXTexSourceDir
$out = Join-Path $root $OutputDir

Remove-Item $out -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $out | Out-Null

$exe = Get-ChildItem -Path $build -Filter ArenaDDSOptimizer.exe -Recurse | Select-Object -First 1
if (-not $exe) { throw "ArenaDDSOptimizer.exe not found in $build" }
Copy-Item $exe.FullName $out

$texconv = Get-ChildItem -Path $dxBuild -Filter texconv.exe -Recurse | Select-Object -First 1
if (-not $texconv) { throw "texconv.exe not found in $dxBuild. Run scripts/build_directxtex.ps1 first." }
Copy-Item $texconv.FullName (Join-Path $out "texconv.exe")

$dxLicense = Join-Path $dxSource "LICENSE"
if (-not (Test-Path $dxLicense)) { throw "DirectXTex LICENSE not found: $dxLicense" }
Copy-Item $dxLicense (Join-Path $out "DirectXTex-LICENSE.txt")

$deploy = Get-Command windeployqt.exe -ErrorAction Stop
& $deploy.Source --release --no-translations --compiler-runtime (Join-Path $out "ArenaDDSOptimizer.exe")

Copy-Item (Join-Path $root "presets") $out -Recurse
Copy-Item (Join-Path $root "README.md") $out
Copy-Item (Join-Path $root "README_RU.md") $out
Copy-Item (Join-Path $root "CHANGELOG.md") $out
Copy-Item (Join-Path $root "THIRD_PARTY_NOTICES.md") $out
if (Test-Path (Join-Path $root "LICENSE")) { Copy-Item (Join-Path $root "LICENSE") $out }

Write-Host "Portable package: $out"
Write-Host "Bundled DirectXTex texconv: $($texconv.FullName)"
