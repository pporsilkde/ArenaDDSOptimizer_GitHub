# Arena DDS Optimizer

Windows Qt/C++ batch DDS analyzer and optimizer for ArenaMP / ArenaMW texture packs. The application is built for **Windows x64 only**, while its texture profiles can target both Windows and Android/ng-gl4es.

## Windows portable release

GitHub Actions produces one portable Windows ZIP containing:

- `ArenaDDSOptimizer.exe`;
- required Qt runtime DLLs;
- **Microsoft DirectXTex `texconv.exe`**;
- `DirectXTex-LICENSE.txt`;
- presets and documentation.

End users do **not** need to install Texconv separately or configure `winget`. The application automatically prefers `texconv.exe` located next to `ArenaDDSOptimizer.exe`.

The repository does not commit a prebuilt third-party executable. CI clones the official Microsoft DirectXTex repository at the pinned `may2026` tag, builds the `texconv` target, and adds the resulting executable plus the upstream MIT license to the portable package.

## Features

- Recursive DDS processing through arbitrarily nested subdirectories.
- Click any table header to sort by name, resolution, format, mip count, real file size, target, plan, or status.
- **Additional compression**: Normal / Strong / Maximum. Because BC1/BC3 use fixed-size blocks, extra size reduction is performed by a controlled maximum-resolution cap for oversized textures while retaining GPU-native DDS formats and a full mip chain.
- Windows portable builds bundle the official Microsoft DirectXTex `texconv.exe`.

- Recursive DDS scanning through all nested subdirectories.
- Preserves relative subdirectory layout in the output and backup trees.
- Reads DDS/DX10 headers without fully decoding every image during analysis.
- Safe Windows + Android profile based on BC1/DXT1 and BC3/DXT5.
- Android/ng-gl4es performance profile with a lower maximum texture size.
- Windows quality and optional Windows BC7 profiles.
- Mipmap generation, resizing, and block compression through bundled DirectXTex Texconv.
- Dry-run planning before modifying files.
- In-place optimization with timestamped `_ArenaDDS_Backup` copies.
- Excludes its own backup tree and an output directory located inside the source tree from recursive rescanning.
- Progress display and cancellation.

## Bundled DirectXTex Texconv

The project uses Microsoft DirectXTex Texconv for the actual texture conversion work. Windows CI pins:

```text
DirectXTex tag: may2026
```

The portable release places:

```text
ArenaDDSOptimizer.exe
texconv.exe
DirectXTex-LICENSE.txt
```

in the same directory. The Texconv path field remains available only as a developer/fallback override.

## Build on Windows

Requirements:

- Windows 10/11 x64;
- Visual Studio 2022 Build Tools / MSVC;
- CMake 3.21+;
- Ninja;
- Git;
- PowerShell 7 (`pwsh`);
- Qt 6.x Widgets (GitHub CI uses Qt 6.8.3).

Run:

```bat
BUILD_WINDOWS.bat
```

This script builds the pinned DirectXTex Texconv first, then Arena DDS Optimizer, and finally creates:

```text
package\windows\
```

with the complete portable application.

## GitHub Actions

- **Build Windows**: runs on pushes and pull requests and uploads `ArenaDDSOptimizer-Windows-x64.zip`.
- **Release Windows**: a tag such as `v0.1.4` builds the Windows package and creates a GitHub Release.
- **CodeQL Windows**: C/C++ analysis on Windows.

## ArenaMP / ArenaMW workflow

1. Select the source `Textures` folder.
2. Keep **recursive subdirectories** enabled.
3. Choose `Windows + Android — safe` for a shared texture pack, or the Android/ng-gl4es profile for a mobile-specific pack.
4. Scan and review files marked for manual inspection.
5. Optimize to a separate output directory first.
6. Test on the target renderer.
7. Optionally pack the resulting tree into your ArenaMP/ArenaMW BSA pipeline.

Detailed Russian documentation: [README_RU.md](README_RU.md).

## Third-party software

See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). DirectXTex / Texconv is provided under the Microsoft MIT license; the Windows package includes the exact upstream license text used by the pinned checkout.

## License

Arena DDS Optimizer source code: GPL-3.0. See `LICENSE`.


### About additional compression

BC1/BC3 are fixed-rate block formats, so a smaller DDS cannot be produced merely by increasing a generic compression level. **Strong** and **Maximum** only lower the resolution cap for oversized textures, preserve a full mip chain, and enable DirectXTex dithering for BC1/BC3. The application shows a warning before applying these lossy size-reduction modes.

## v0.1.4 texture protection and resize controls

The GUI can resize by the shorter side to 256/512/1024/2048 without upscaling, skip small textures (shorter side <=256), and fully exclude configurable filename/path patterns such as `menu` and `tx_scroll`.
