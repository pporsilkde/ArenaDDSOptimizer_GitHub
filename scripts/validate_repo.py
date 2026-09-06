from pathlib import Path
import json
root = Path(__file__).resolve().parents[1]
required = [
    'CMakeLists.txt','CMakePresets.json','src/main.cpp','src/mainwindow.cpp',
    '.github/workflows/build.yml','.github/workflows/release.yml',
    'README.md','README_RU.md','LICENSE'
]
missing=[x for x in required if not (root/x).exists()]
if missing:
    raise SystemExit('Missing: ' + ', '.join(missing))
json.loads((root/'CMakePresets.json').read_text())
print('Repository structure: OK')
