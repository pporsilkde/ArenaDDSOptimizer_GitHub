param(
    [string]$Tag = "may2026",
    [string]$SourceDir = "third_party/DirectXTex",
    [string]$BuildDir = "build/directxtex"
)

$ErrorActionPreference = "Stop"
$root = (Resolve-Path "$PSScriptRoot/..").Path
$source = Join-Path $root $SourceDir
$build = Join-Path $root $BuildDir

if (-not (Test-Path (Join-Path $source ".git"))) {
    New-Item -ItemType Directory -Force -Path (Split-Path $source -Parent) | Out-Null
    Write-Host "Cloning Microsoft DirectXTex tag $Tag..."
    git clone --depth 1 --branch $Tag https://github.com/microsoft/DirectXTex.git $source
} else {
    Write-Host "Using existing DirectXTex checkout: $source"
}

# Ensure the checkout is the requested reproducible tag.
git -C $source fetch --depth 1 origin "refs/tags/$Tag:refs/tags/$Tag"
git -C $source checkout --force $Tag

cmake -S $source -B $build -G Ninja `
    -DCMAKE_BUILD_TYPE=Release `
    -DBUILD_TOOLS=ON `
    -DBUILD_SAMPLE=OFF `
    -DBUILD_SHARED_LIBS=OFF `
    -DBUILD_DX11=ON `
    -DBUILD_DX12=OFF `
    -DBC_USE_OPENMP=ON `
    -DENABLE_OPENEXR_SUPPORT=OFF `
    -DENABLE_LIBJPEG_SUPPORT=OFF `
    -DENABLE_LIBPNG_SUPPORT=OFF

cmake --build $build --target texconv --config Release --parallel

$texconv = Get-ChildItem -Path $build -Filter texconv.exe -Recurse | Select-Object -First 1
if (-not $texconv) {
    throw "DirectXTex build succeeded but texconv.exe was not found under $build"
}

Write-Host "texconv.exe: $($texconv.FullName)"
Write-Output $texconv.FullName
