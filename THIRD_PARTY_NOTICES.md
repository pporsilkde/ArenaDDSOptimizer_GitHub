# Third-party notices

## Microsoft DirectXTex / texconv

Arena DDS Optimizer uses the `texconv.exe` command-line utility from Microsoft DirectXTex for texture conversion.

- Upstream: https://github.com/microsoft/DirectXTex
- Pinned release used by Windows CI: `may2026` (May 7, 2026)
- License: MIT

The Windows portable package includes `DirectXTex-LICENSE.txt`, copied from the pinned upstream source checkout at build time.

`texconv.exe` is built by GitHub Actions from the official DirectXTex source; no prebuilt third-party executable is committed to this repository.
