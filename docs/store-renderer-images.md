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

## Image memory accounting and resource creation

Two further functions have been reconstructed, with provisional Store ABI
names (not claims of recovered original source identifiers):

| Function | MP VA | ZM VA | SP VA |
| --- | --- | --- | --- |
| Store_ImageTrackMemory | 140776EA0 | 1407749E0 | 14061C790 |
| Store_ImageMipBytes | 140776FF0 | 140774B30 | 14061C8E0 |

Their contiguous code and full jump table match across the three originals
after normalizing only image-relative address operands. The mip-size
routine uses unsigned 32-bit arithmetic and returns zero for unsupported
formats. The caller reads picmip bytes at +0xC and +0xD and writes two
32-bit memory counters at +0x10 and +0x14. Flags 1, 2 and 4 respectively
bypass picmip, suppress the mip chain, and multiply total size by six.
The full record has not yet been recovered; the implementation touches
only the observed fields. These values are engine accounting values, not
actual Vulkan allocation sizes.

    python tests/verify_store_image_memory.py /path/to/store/files

Executed on Linux x86-64 with the original jump tables intact:
13,500 mip-byte comparisons and 8,000 whole-buffer accounting comparisons
per executable, 64,500 total. Windows x64 COFF object compilation also
passed. No GPU calls or service calls are involved. These tests are not
registered in the standard differential runner and do not assert full
engine integration.

The regular upload format-switch index table (76 entries) and all six
relative destination offsets are identical across MP/ZM/SP. The original
mip-size table differs from this upload-pitch table: formats 10 and 11
account for eight bytes per texel but the regular upload path uses four
bytes per width unit. Do not use the accounting or upload switch as a
universal Vulkan format-size table.

The setup callee's three resource creation destinations are also identified
by branch flags, the image type byte written at +8, and D3D11 device
vtable slots (verified in all three PEs):

| Candidate role | Image type | MP VA | ZM VA | SP VA |
| --- | --- | --- | --- | --- |
| Image_Create2DTexture_PC | 3 | 14073B7D0 | 140739300 | 1405DE8C0 |
| Image_Create3DTexture_PC | 4 | 14073B990 | 1407394C0 | 1405DEA80 |
| Image_CreateCubeTexture_PC | 5 | 14073BAA0 | 1407395D0 | 1405DEB90 |

Each uses CreateShaderResourceView (+0x38) following CreateTexture2D
(+0x28) or CreateTexture3D (+0x30). They have equal corresponding unwind
body lengths (437/258/248 bytes respectively) and the same type writes.
No full descriptor, error, or resource lifetime parity is claimed yet;
these are not bound by this commit.

Two propagated MP matcher labels also merit review: R_SetViewportStruct
at 1407645E0 initializes a 0x1B70-byte source-state object and copies
0xF30 bytes of input before resetting constant data, unlike the
reference's small viewport setter; R_GetClearColor at 140719390 performs
render-target/state operations and takes a second input, unlike the
reference's simple clear-color getter. Do not adopt the current generated
labels as verified ABI mappings. The generated analysis is unchanged.

The next native renderer work is descriptor extraction and exact texture
upload semantics, then render-target lifecycle, shader bindings and
command submission. Keep original memory accounting separate from Vulkan
device memory requirements and allocation alignment.

## Exact resource descriptor reconstruction

The three resource creation candidates have identical contiguous code
across MP/ZM/SP after relocating 15 verified RIP-relative global
references. Isolated replay captures the texture and SRV descriptors,
the initial-data pointer, writes to the image type/dimensions and
resource creation/view/release order. The retry flag is zero in this
test; failure/retry behavior is not reconstructed.

| Constructor | 2D | Volume | Cube |
| --- | --- | --- | --- |
| Image type | 3 | 4 | 5 |
| Texture descriptor size | 44 | 36 | 44 |
| Array layers | 1 | n/a | 6 |
| Usage | DEFAULT | DYNAMIC | DEFAULT |
| CPU access | 0 | WRITE | 0 |
| SRV dimension | 2D or 2DMS | 3D | CUBE |

2D flags 0x20000 and 0x40000 control attachment bind flags and
multisampling respectively. Formats 20/40/45 use typeless resource
formats 19/39/44 and SRV formats 21/41/46; format 27 uses SRV 28.
The other 2D formats keep their original format, and 3D/cube use
their given format without remapping. All dimensions are truncated to
16 bits, exactly as in the original. Mip count is passed through.
The cube path sets the texture-cube misc flag. The 3D path has an
unused sixth argument slot (caller setup leaves it unspecified).

The new Store_DescribeImage2D/3D/Cube helpers reproduce only the
descriptor and image-metadata construction portion, not the original
COM functions themselves. They intentionally have no executable symbol
binding or runtime redirect. Vulkan will use the recovered intent rather
than creating D3D11 descriptors or emulating COM.

    python tests/verify_store_image_descriptor.py /path/to/store/files

Executed successfully on Linux x86-64: 6,000 comparisons per executable
(18,000 total), covering 100 format values, four flag combinations,
five sample counts, random dimensions, mip levels and sample quality.
Original resource creation calls are mocked, so this includes invalid
descriptor combinations and does not establish actual GPU acceptance.
The previous 67,662 constructor and memory-accounting comparisons also
passed again. No actual GPU, full hybrid build or resource-error replay
has been performed. The source tree and generated analysis layout are
unchanged.

## Regular texture upload contract

Store_PlanImageUpload reproduces the decision and arguments of the
regular upload boundary (Store_ImageUploadData), not the actual device
call. The exact switch index and target tables match across all three
pinned originals, as does the body after normalizing image-relative
addresses and the five global references.

| Image type | Behavior |
| --- | --- |
| 3 (2D) | UpdateSubresource if format supported |
| 4 (3D) | Delegate to volume uploader, ignoring face argument |
| 5 (cube) | UpdateSubresource if format supported |

Width is read as uint16, shifted by the x86 masked mip count and
clamped to at least 1. The subresource index is face * actual resource
descriptor mip count + mip, with 32-bit wrap. Row-pitch cases are
10/11/28/35/41 = 4*width, 65 = width, 71 = 8*ceil(width/4),
74/77 = 16*ceil(width/4), 85 = 2*width. Other formats skip the GPU
call altogether. The original supplies a null destination box and zero
depth pitch, obtains/releases the resource from its SRV, and enters
and leaves the context critical section around the update.

The volume delegate is at MP 1407770F0, ZM 140774C30, SP 14061C9E0.
Its internal Map/Unmap and per-slice copying have not been reconstructed
yet. Do not use the regular 2D row-pitch table as a volume layout or
a universal Vulkan format table.

    python tests/verify_store_image_upload.py /path/to/store/files

Executed successfully on Linux x86-64: 18,900 comparisons per build
(56,700 total), including unsupported formats, boundary dimensions,
masked shifts, randomized face and descriptor mip counts, and volume
dispatch. Captured GPU arguments and ordering are compared with the
C++ upload plan; actual data transfer, locks and volume copy are
mocked. No runtime binding is installed. The descriptor helpers from
the previous commit also compiled successfully as Windows x64 COFF.

## Full upload bodies (regular and volume)

The earlier Store_PlanImageUpload helper is not a reconstruction of a
complete original function. This pass adds the full Store_ImageUploadData
and Store_ImageUploadVolume bodies in src/gfx_d3d/r_image_upload_native.cpp.
Both have void return; the volume ABI is (image, format, mip, pixels).
The ordinary path obtains/releases the view resource, reads the actual
resource mip count, locks the context, and calls UpdateSubresource.
The volume path maps each slice with WRITE_DISCARD, copies tightly packed
or padded rows, unmaps, advances the source, and releases the resource
after leaving the context lock. It reads the image mip count at +0x1E.
The volume source-slice size and row pitch use distinct format switches:
45,55,83 can advance the source without copying; 10,11,35 can copy rows
without advancing the slice source. Preserve this original behavior.
Unsupported volume formats still Map/Unmap. Original Map HRESULT is
ignored; failed maps are not safe to exercise and are not validated.

    python tests/verify_store_image_full_upload.py /path/to/store/files

Executed against SHA-256-pinned MP, ZM and SP originals on Linux x86-64:
15,000 full-path comparisons per build (45,000 total), with 100 formats,
five dimension triples, five mip indices and tight/padded mapped rows.
Mocked COM calls capture exact destination bytes including padding,
subresource numbers, pitch and call order. The real original dispatch
and both original jump tables execute. Successful Map is mocked, as are
GetResource, GetDesc, UpdateSubresource, lock, release and the CRT copy.
No GPU, complete Windows hybrid build or runtime integration is claimed.
These results supersede the upload-plan-only status. Descriptor-only
helpers and the original setup body remain incomplete.

For the standard reconstruction ledger, the full upload bodies remain
outside records/functions.tsv until the standard differential runner
supports this test. The symbol names are provisional Store ABI labels.

## Complete resource constructors and setup

This pass supersedes the descriptor-only status. The three resource
constructors and Store_ImageSetup now have full C++ bodies in
src/gfx_d3d/r_image_create.cpp. All four return void; the 3D constructor
has an unused sixth argument slot, retained for its Store ABI. Resource
creation and SRV creation each loop until the shared engine retry flag
is clear (not based on HRESULT), then release the temporary resource.
The original does not handle a failed final creation that leaves an
invalid resource pointer; this is not claimed safe.

Setup writes truncated dimensions even when disabled. It reads a dvar
pointer (original name not yet recovered) with Dvar_GetBool semantics,
selects cube before volume before 2D, derives mip levels from bit 1,
and calls the reconstructed memory-accounting function using the original
untruncated dimensions. The unused volume argument is explicitly zero in
the authored body; no observable effect was found.

    python tests/verify_store_image_create.py /path/to/store/files

Executed on Linux x86-64 with the three SHA-256-pinned original PEs:
20,000 comparisons per build, 60,000 total. The four original bodies
execute together with mocked D3D11 COM calls, atoi and accounting
boundary; native and original image bytes, descriptor words, initial
data pointer, accounting arguments and order/count of calls are checked.
The test exercises null/bool/string dvars, 100 formats, ten flag sets,
random dimensions and multisample values, null/non-null initial data
and one to three calls per retry loop. Original bodies normalize
identically across builds after verified address relocation. The native
test uses a local equivalent copy of the existing descriptor helpers;
the checked-in test uses the repository helpers. Windows x64 COFF
compilation passed against the isolated test headers, not the complete
hybrid build. Actual GPU allocation and terminal COM failures remain
unvalidated. No enhanced redirects installed, no standard ledger
promotion before its runner supports these tests.

## Complete image release and render-target shutdown

| Function | MP | ZM | SP |
| --- | --- | --- | --- |
| Image_Release | 14073B6B0 | 1407391E0 | 1405DE7A0 |
| sub_140753660 (render-target shutdown) | 140753660 | 1407511A0 | 1405F9F80 |

Both complete bodies normalize identically across the three pinned
originals after relocation. Image_Release subtracts two unsigned
memory counters when the track byte is greater than one and not four,
releases the view if present, clears its pointer and both card-memory
fields only if the view existed, subtracts loaded bytes, and resets
loaded size and map type. The original retains a string conversion
call in the no-view, non-delayed, non-boolean dvar branch. Its result
is ignored. The reference identifies the dvar as r_loadForRenderer;
the exact Store registration is not verified here.

Shutdown iterates 72 32-byte records, releases color then depth view
then image when non-null, zeroes the complete record, releases three
additional images in fixed order, and zeros an eight-byte state field.
The original source name of shutdown and the semantic names of the
three additional images and state are not yet recovered.

    python tests/verify_store_render_release.py /path/to/store/files

Executed successfully on Linux x86-64: 5,120 isolated image-release
cases and 1,280 complete shutdown cases per executable, 19,200 total.
Tests compare original and authored complete image bytes, counters,
ordered release/conversion calls, and zeroed target records.
COM Release and atoi are mocked; no actual GPU, full hybrid build,
or runtime redirects are claimed. The standard ledger is unchanged
until its runner supports the separate replay tests. No original
instruction bytes are committed.
