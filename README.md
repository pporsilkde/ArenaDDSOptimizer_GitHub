# Arena DDS Optimizer

Qt/C++ batch DDS analyzer and optimizer for ArenaMP/ArenaMW texture packs, with conservative Windows + Android/ng-gl4es presets.

## Features

- Recursive DDS inspection without decoding the full image.
- BC1/DXT1 and BC3/DXT5 compatibility-oriented profiles.
- Mipmap generation and resize through an external DirectXTex `texconv`.
- Dry-run planning, backup-on-in-place processing, progress and cancellation.
- Windows x64 portable ZIP and Linux x86_64 AppImage built by GitHub Actions.
- Tag `vX.Y.Z` to automatically publish a GitHub Release.

`texconv` is intentionally not bundled. Point the application to your own DirectXTex `texconv.exe`.

See [README_RU.md](README_RU.md) for the detailed Russian documentation and [docs/GITHUB_SETUP_RU.md](docs/GITHUB_SETUP_RU.md) for repository setup.

## Build

Requirements: CMake 3.21+, Ninja, C++17, Qt 5.15+ or Qt 6.x Widgets.

```bash
cmake --preset release
cmake --build --preset release --parallel
```

## License

GPL-3.0.
