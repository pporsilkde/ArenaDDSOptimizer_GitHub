## 0.1.5

- Shorter-side resizing is now an exclusive resolution mode: when 256/512/1024/2048 is selected, no profile/long-side resize is applied.
- Files whose shorter side is already at or below the selected target are never resized or upscaled.
- Small textures and configurable name patterns (`menu`, `tx_scroll`, etc.) now protect resolution only; other optimization steps may still run.
- CodeQL no longer launches a second Windows build on every push; it runs weekly or manually.
- Windows EXE now embeds the Arena DDS Optimizer icon.
- Portable packaging keeps the MSVC runtime for portability, omits the large unused Qt software-OpenGL fallback, prunes unused Qt plugin directories, and GitHub Actions uses maximum ZIP compression.

# 0.1.4

- Adds optional resizing by the **shorter texture side** to 256 / 512 / 1024 / 2048 while preserving aspect ratio and never upscaling.
- Adds automatic protection for small textures: by default DDS with a shorter side of 256 px or less are skipped.
- Adds configurable filename/path exclusions. Defaults include `menu`, `tx_scroll`, `icons`, `gui`, `hud`, `cursor`, and `font`.
- Excluded textures are fully skipped, including forced re-encode, to avoid damaging UI assets.
- The profile maximum dimension remains a hard upper bound in addition to the new shorter-side target.

# 0.1.3

- Adds user-selectable additional DDS size reduction: Normal / Strong / Maximum.
- Strong and Maximum modes lower only the maximum resolution cap for oversized textures while keeping BC1/BC3 GPU-native compression and full mipmaps.
- Adds sortable texture table: click any column header to sort ascending/descending.
- File size sorting uses real byte counts rather than formatted text, so 900 KB correctly sorts below 1.2 MB.
- Resolution and mip columns use numeric sorting.
- Sorting is safe during optimization; job-to-row tracking no longer relies on fixed table row numbers.

# 0.1.2

- Windows-only GitHub build and release pipeline.
- Bundles Microsoft DirectXTex `texconv.exe` in the portable package.
- `texconv` is built reproducibly from the official `may2026` tag.
- The optimizer automatically prefers the bundled `texconv.exe` next to the application.
- Adds DirectXTex MIT license and third-party notice to the release.
- Removes Linux/AppImage CI and packaging.

# Changelog

## 0.1.1

- Clarified recursive scanning as unlimited-depth traversal of all nested subdirectories.
- Recursive scanning remains enabled by default.
- Output keeps the complete relative directory tree.
- Excludes `_ArenaDDS_Backup` and an output directory located inside the source tree from scanning.

## 0.1.0

- Qt 5/6 Widgets GUI.
- Recursive DDS header scan (legacy FOURCC + DX10 common BC formats).
- Built-in Windows+Android, Android, Windows, and advanced Windows BC7 profiles.
- Conservative DXT1/DXT5 planning for Arena/OpenMW compatibility.
- Normal-map filename heuristics.
- Maximum-dimension resize planning.
- Full mip-chain generation through DirectXTex texconv.
- Relative directory preservation.
- In-place backup and atomic output replacement.
- Dry-run/analysis mode.
- Refuses automatic reinterpretation of BC4/BC5/BC6H/BC7 in compatibility profiles.
