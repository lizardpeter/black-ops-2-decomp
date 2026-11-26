# Store client-game dvar registration (MP)

Candidate authored reconstruction: `src/cgame_mp/cg_register_dvars_store.cpp`.

Target is the Store MP image, not the older x86 PC server. Entry 140101580; final tail jump at 140103C9B to 140101410. The generated Ghidra pseudocode and disassembly were both examined. The candidate has 180 registration operations (25 inline integer lookup/new-or-reregister branches), 125 global pointer assignments, 7 enum-table bindings, and 10 subsidiary calls (including the final tail callee, excluding the initial script-mode query). Static checks compared the ordered direct-call/lookup sequence and the complete multiset of global write addresses to the MP disassembly; both matched. The omitted fourth argument in the Ghidra cg_thirdPersonMode enum call is 0x80, confirmed by MOV R9D at 1401023A4.

This is a complete source-body candidate, not a differential pass or runtime integration. It is opt-in with T6_STORE_CG_PROJECT_REPLAY so unresolved aliases cannot accidentally affect the ordinary hybrid build. The new provisional callee/global names must be bound and replayed against the pinned PE (with both existing and new registrations) before promotion. ZM and SP have only PE and pdata inventories checked in, not full disassembly; no cross-mode equivalence is claimed.

## Store MP callee bindings

| Alias | Address |
| --- | --- |
| Store_CG_ExitAfterConnectPaths | 140297EB0 |
| Store_CG_FindDvar | 14046B8B0 |
| Store_CG_RegisterNew | 14046C1A0 |
| Store_CG_Reregister | 14046BF10 |
| Store_CG_Bool | 14046C4C0 |
| Store_CG_Float | 14046C650 |
| Store_CG_Vec3 | 14046C840 |
| Store_CG_Enum | 14046CAF0 |
| Store_CG_String | 14046CA30 |
| Store_CG_Vec2 | 14046C760 |
| Store_CG_Color | 14046CBD0 |
| Store_CG_Sub_1400fb7d0 | 1400FB7D0 |
| Store_CG_Sub_14022ad80 | 14022AD80 |
| Store_CG_Sub_1400c8290 | 1400C8290 |
| Store_CG_Sub_1400ba5a0 | 1400BA5A0 |
| Store_CG_Sub_1400f7b80 | 1400F7B80 |
| Store_CG_Sub_1400f2230 | 1400F2230 |
| Store_CG_Sub_14015df60 | 14015DF60 |
| Store_CG_Sub_140179890 | 140179890 |
| Store_CG_Sub_14061baf0 | 14061BAF0 |
| Store_CG_Sub_140101410 | 140101410 |

Enum table and global addresses are alongside declarations in the candidate. Values are represented as exact IEEE-754 bits to avoid decimal rounding.

## FOV ownership

MP CG_RegisterDvars_Project stores cg_fov at 140E4F8E8 (65/1/160), cg_fovScale at 140D78658 (1/0.2/2), cg_fovMin at 140D78648 (10/1/160), and cg_fovExtraCam at 140D78638 (30/1/160), all flags 0x80. The user-facing cg_fov_default (65/65/90, flags 0x84000, global 142161688) and cg_fov_default_thirdperson (40/1/160, flags 0x84000, global 142161678) are registered in the much larger Windows startup routine 14049BFC0, not in CG_RegisterDvars_Project. The reference client code confirms a modified callback, third-person view selection and camera-specific FOV paths; those must be traced in the Store image before claiming persistence or raising the cap in enhanced. No startup or enhanced runtime change is made here.

## Direct FOV global reference scan

Run 45893707622 scanned the pinned original and confirmed three cg_fovScale RIP-relative references: 140101898 (write in registration 140101580), 140101562 (read in 140101410), and 1400C1FDA (read in 1400C1B90). cg_fovMin has two references and cg_fov has 16. This is a direct reference inventory, not a claim of all indirect consumers or complete camera math. The next extraction captures both cg_fovScale consumer bodies.
