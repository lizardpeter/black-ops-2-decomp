# Black Ops II decompilation

Call of Duty: Black Ops II (T6), PC x86. The retail multiplayer target is `t6mp`. The March 2013 PC dedicated server supplies the first symbol-backed reference. See [build provenance](docs/builds.md) and [function status](STATUS.md).

- `t6_faithful` reconstructs original behavior. `t6_enhanced` builds from those same sources and can replace individual translation units through [enhanced/sources.cmake](enhanced/sources.cmake).
- [AGENTS.md](AGENTS.md) defines where generated analysis, source, tests, and evidence go.
- [Ghidra analysis](.github/workflows/ghidra.yml) exports disassembly and pseudocode for a selected build. The artifacts contain analysis outputs. The originals remain in R2.

## Build and compare the first functions

```sh
python -m pip install -r tests/differential/requirements.txt
python tools/fetch_originals.py CoDMPServer_PC originals --only exe
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
python tests/differential/reference.py originals/CoDMPServer_PC.exe build/libt6_faithful.so
python tests/differential/reference.py originals/CoDMPServer_PC.exe build/libt6_enhanced.so
```

These commands build reconstruction libraries and compare their functions with original x86 routines under Unicorn.
