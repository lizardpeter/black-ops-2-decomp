# Black Ops II decompilation

Call of Duty: Black Ops II (T6). The target is the Microsoft Store build (x64, VS2019, GDK): multiplayer
`t6mp_msstore` first, then zombies and campaign. The March 2013 dedicated servers (x86, full PDBs, PC MAPs) are the
references; the 2015 Steam `t6mp.exe` is a related helper. See [the Store build](docs/store-build.md),
[build provenance](docs/builds.md), [matching](docs/matching.md) and [function status](STATUS.md).

- The faithful build (`src/`) reconstructs original behaviour, compiled into a DLL hybrid-linked against each original exe and tested function by function ([verification](docs/verification.md)). The enhanced build adds overrides from `enhanced/` ([roadmap](docs/roadmap.md)).
- [AGENTS.md](AGENTS.md) defines where generated analysis, source, tests, and evidence go.
- [Ghidra analysis](.github/workflows/ghidra.yml) exports disassembly and pseudocode for a selected build. The artifacts contain analysis outputs. The originals remain in R2.

## Build and compare the first functions

```sh
sudo apt-get install clang lld llvm
pip install -r tests/differential/requirements.txt
for b in CoDMPServer_PC t6mp t6mp_msstore; do python tools/fetch_originals.py $b originals --only exe; done
for b in CoDMPServer_PC t6mp t6mp_msstore; do python tools/build_recon.py --build $b; done
for b in CoDMPServer_PC t6mp t6mp_msstore; do python tests/differential/run.py --build $b --check-records; done
```

These commands build reconstruction libraries and compare their functions with the original routines (x86 or x64) under Unicorn.
