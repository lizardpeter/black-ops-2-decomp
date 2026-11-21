# Black Ops II decompilation

Call of Duty: Black Ops II (T6). The target is the Microsoft Store build (x64, VS2019, GDK): multiplayer
`t6mp_msstore` first, then zombies and campaign. The March 2013 dedicated servers (x86, full PDBs, PC MAPs) are the
references; the 2015 Steam `t6mp.exe` is a related helper. See [the Store build](docs/store-build.md),
[build provenance](docs/builds.md), [matching](docs/matching.md) and [function status](STATUS.md).

- `t6_faithful` reconstructs original behavior. `t6_enhanced` builds from those same sources and can replace individual translation units through [enhanced/sources.cmake](enhanced/sources.cmake).
- [AGENTS.md](AGENTS.md) defines where generated analysis, source, tests, and evidence go.
- [Ghidra analysis](.github/workflows/ghidra.yml) exports disassembly and pseudocode for a selected build. The artifacts contain analysis outputs. The originals remain in R2.

## Build and compare the first functions

```sh
python -m pip install -r tests/differential/requirements.txt
python tools/fetch_originals.py CoDMPServer_PC originals --only exe
python tools/fetch_originals.py t6mp originals --only exe
python tools/fetch_originals.py t6mp_msstore originals --only exe
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
python tests/differential/reference.py originals/CoDMPServer_PC.exe build/libt6_faithful.so
python tests/differential/reference.py originals/CoDMPServer_PC.exe build/libt6_enhanced.so
python tests/differential/reference.py originals/t6mp.exe build/libt6_faithful.so --build t6mp
python tests/differential/reference.py originals/t6mp.exe build/libt6_enhanced.so --build t6mp
python tests/differential/reference.py originals/t6mp_msstore.exe build/libt6_faithful.so --build t6mp_msstore
python tests/differential/reference.py originals/t6mp_msstore.exe build/libt6_enhanced.so --build t6mp_msstore
```

These commands build reconstruction libraries and compare their functions with the original routines (x86 or x64) under Unicorn.
