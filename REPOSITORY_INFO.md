# Repository profile

Suggested GitHub repository name: `ArenaDDSOptimizer`

Description: `Windows Qt DDS texture analyzer/optimizer for ArenaMP and ArenaMW with bundled Microsoft DirectXTex Texconv.`

Topics: `qt`, `windows`, `dds`, `directxtex`, `texconv`, `openmw`, `tes3mp`, `morrowind`, `texture-optimization`, `android`, `ng-gl4es`

Default branch: `main`

CI:
- Build Windows: Windows x64 portable ZIP on push/PR.
- DirectXTex: CI builds pinned tag `may2026` and bundles `texconv.exe`.
- Release Windows: tag `vX.Y.Z` creates a GitHub Release with the Windows ZIP.
- CodeQL: Windows C/C++ security analysis on push/PR and weekly.
