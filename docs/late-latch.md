# Late latch: the view path from the mouse to the GPU

The plan for the Store build's CPU late latch (docs/roadmap.md, "Full late latch"): on the render thread, just
before the frame's 3D views are drawn, turn the camera by the raw mouse counts that arrived after the game frame read
the mouse. The counts stay with the game, which takes them on its next frame as usual (Forge's `peek_motion()`, BO3's
P0021, whose map in black-ops-3-decomp docs/late-latch.md this follows). BO2 has an advantage over BO3: the server's
PDB names the whole path, so the Store functions can be identified by their names' shapes.

## The path (server names; Store addresses where identified)

| Step | Server | Store MP |
|---|---|---|
| The view: angles to axis | `CG_CalcViewValues` 004C3880 (cgame_mp/cg_view_mp.cpp) | 14016C510 (matcher, caller) |
| The frame's cgame draw, ending in the scene | `CG_DrawActiveFrame` tail, cgame_mp/cg_draw_mp.cpp | to identify |
| Hand-off to the renderer, with a **draw view and a separate culling (DPVS) view** | `CL_RenderScene(fd, frameTime, viewParmsDraw, viewParmsDpvs)` 004F09C0 | to identify |
| View parms from the refdef: `MatrixForViewer`, `InfinitePerspectiveMatrix`, view×projection and its inverse | `R_SetViewParmsForScene` 00A1D1F0, `R_SetupViewProjectionMatrices` 00A11820 | to identify |
| Scene setup and culling | `R_RenderScene` 00A1E280, `R_GenerateSortedDrawSurfs` 00A1AE70 | `R_GenerateSortedDrawSurfs` 140708320 |
| Hand-off to the render thread | `R_IssueRenderCommands` 00A2F1E0, `R_ToggleSmpFrame` 00A2B920 | `R_ToggleSmpFrame` 14071BFA0 |
| Render thread: the 3D views | `RB_Draw3D` 00A5A9D0 → `RB_Draw3DInternal` 00A28130 → `RB_StandardDrawCommands` | `RB_Draw3D` 140736120, `RB_StandardDrawCommands` 140718370 |
| View parms into the draw state; matrix versions bumped | `R_Set3D` 00A8C420 (copies the 0x150-byte `GfxViewParms`) | to identify |
| Shader matrices, derived when their version is behind | `R_GetCodeMatrix` 00A8D390, `R_DeriveCodeMatrix` 00A8D2C0 | to identify |

`GfxViewParms` is 0x150 bytes with the same layout in both builds (no pointers): view matrix +0x00, projection +0x40,
view×projection +0x80, its inverse +0xC0, origin +0x100, axis +0x110, depth-hack near clip +0x134, zNear +0x138.
`GfxViewInfo` is 0xC40 bytes on the server, 0xC60 predicted for x64.

## The latch (to build)

1. **Main thread.** The enhanced `CL_MouseMove` already turns the view by raw counts (P0002/P0003); it also notes
   the counts the frame took and the degrees per count, and whether the mouse turned the view this frame (not in
   menus, vehicles, killcams, turret limits). The culling view (`viewParmsDpvs`, already separate in BO2) is widened
   by a margin so a late turn finds nothing culled at the edges.
2. **Render thread, at the start of `RB_Draw3D`.** Turn = (counts now − counts the frame took) × degrees per count,
   capped at the margin; yaw about world up, pitch about the view's right, pitch stopped at the game's limit. The
   main view's `GfxViewParms` is made again exactly as `R_SetViewParmsForScene` would from the turned axis.
3. **The gun.** Placed on the main thread from the unturned view; drawn with the depth-hack projection. It must turn
   with the camera, or it trails for a frame: to find how BO2 draws it (the depth hack in `R_DeriveCodeMatrix` /
   `R_Set3D`) and handle it as BO3's P0021 does.
4. Each hooked function is reconstructed faithfully and verified on the Store first; the latch goes in `enhanced/`
   with an `r_lateLatch` dvar (default on) and a patch in `records/patches.tsv`.
