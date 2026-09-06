# Contributing

1. Create a feature branch from `main`.
2. Keep the project compatible with Qt 5.15+ at source level unless a change explicitly raises the minimum.
3. Do not commit prebuilt `texconv` or other third-party binaries. Windows CI must build the pinned official DirectXTex source and bundle the resulting `texconv.exe` in release artifacts.
4. Run a Release build before submitting a pull request.
5. Preserve the conservative Windows+Android texture profile unless a compatibility change is documented.
