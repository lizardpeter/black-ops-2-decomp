# Store renderer reconstruction: built-in images

All three original Store PEs were SHA-256 checked against target.toml. The MP
matcher anchors the built-in image group to the named reference functions.
Read-only original PE disassembly established the Store-specific arguments.
A scan of all unwind entries in each original found one candidate per
constructor when only known relative call displacements were masked.
The three groups share identical normalized bodies and their call targets
resolve consistently within each executable.

| Function/group | MP VA | ZM VA | SP VA |
| --- | --- | --- | --- |
| Image_Generate2D (specialized) | 14076F650 | 14076D190 | 14061BC10 |
| Seven default loaders (stride 0x20) | 14076F6E0 | 14076D220 | 14061BCA0 |
| 23 distinct lightmap-intensity loaders (stride 0x70) | 14076F7C0 | 14076D300 | 14061BD80 |
| Image setup boundary (provisional name) | 1407776A0 | 1407751E0 | 14061CF90 |
| Image upload boundary (provisional name) | 140777440 | 140774F80 | 14061CD30 |

The retained Store Image_Generate2D accepts an image pointer and four
byte-valued color arguments, unlike the reference's general pixel-buffer
prototype. It sets up a 1x1x1 RGBA8 UNORM image (format 28, flags 3)
and uploads one pixel to face/mip zero. The 1x4 lightmap-intensity images
contain white RGB and floor(255/N) alpha in the first texel, followed by
three zero texels. Several registrations share the same original body.
The exact identity-normal texel is (128,128,255,128); linear gray is
(180,180,180,128). Preserve these values rather than assuming full alpha.

## Reproduction

On Linux x86-64 with g++ and the original files named t6mp.exe, t6zm.exe,
t6sp.exe in a local directory:

    python tests/verify_store_image_builtin.py /path/to/store/files

Executed successfully: 1054 comparisons per build, 3162 total. The replay
copies only the 31 small constructor bodies into local executable memory,
redirects their two GPU-facing callees to capture stubs, and compares
setup/upload arguments and all payload bytes against authored C++.
The source also compiles as a Windows x64 COFF object using clang.
This is not a full Windows hybrid link or GPU rendering test. No original
code bytes are committed. No function is marked matches in records/functions.tsv:
the separate test is not yet wired into the repository's standard differential
runner. No enhanced redirects are installed.

## Next backend boundaries

The setup callee writes 16-bit image width/height/depth at +0x18/+0x1A/+0x1C.
Its resource creation branches on flags 4 and 8 (cube / volume candidates,
respectively); 2D is the remaining branch. The upload callee uses the image's
view, obtains the underlying resource and descriptor, computes subresource
and row pitch, and calls the D3D11 context UpdateSubresource slot (+0x180).
The type-4 branch tail calls a separate upload path. Those GPU-facing bodies
are not reconstructed by this change.

In the MP regular upload branch, the observed DXGI-format switch calculates
pitch from max(1,width>>mip): formats 10,11,28,35,41 multiply by 4; 65 is
unchanged; 71 uses ceil(width/4)*8; 74,77 use ceil(width/4)*16; 85 doubles.
Other switch entries go to an early exit. This is the observed original
path, not a general format-size table (notably format 10). Equivalent
arithmetic and call slots are visible in the ZM and SP upload candidates;
full switch-table and resource-type parity remains to be validated.

Vulkan should reuse the built-in image payload semantics, but replace
the setup/upload implementation with native VkImage allocation, staging
and explicit transitions once the underlying resource contracts are recovered.
No Vulkan implementation or renderer migration is claimed here.
