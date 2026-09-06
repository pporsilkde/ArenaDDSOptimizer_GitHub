# Windows / bundled Texconv validation

Version: 0.1.3

Static checks performed for this source package:

- GitHub build workflow contains only a Windows x64 job.
- GitHub release workflow contains only a Windows x64 job.
- CodeQL runs on Windows.
- CMake explicitly rejects non-Windows targets.
- DirectXTex is pinned to tag `may2026`.
- `scripts/build_directxtex.ps1` builds the `texconv` target from official Microsoft DirectXTex source.
- `scripts/package_windows.ps1` requires and copies `texconv.exe` into the portable directory.
- The DirectXTex MIT license is copied as `DirectXTex-LICENSE.txt`.
- Arena DDS Optimizer prefers `texconv.exe` next to the application executable.
- Recursive DDS scanning remains enabled by default and traverses nested subdirectories.
- The texture table uses typed sorting keys for file bytes, resolution area, and mip counts; row/job association remains stable after sorting.
- Additional compression levels retain BC1/BC3 plus mipmaps and reduce only oversized texture dimensions.
- YAML and CMakePresets JSON parse successfully in the preparation environment.

A full Windows/MSVC/Qt/DirectXTex build cannot be executed in the Linux preparation container; GitHub Actions is configured to perform that build on `windows-2022`.
