# Store MP camera reconstruction

First complete enclosing-body candidate: 1400C1B90..1400C2057 (309 instructions). The implementation preserves weapon selection, override dvar conversions, packed attachment indices, the attachment sentinel and first-null termination, 45-degree fallback, default-FOV and mode early returns, weapon and entity overrides, minimum and scale computation. Several early returns bypass the final scale. The original pointer at 140E4F8E0 is indexed by client, but the adjacent address 140E4F8E8 is also cg_fov; only client zero is asserted in the test. The first ABI lane to 1400C12F0 and original error boundary need further callee evidence. The differential harness controls original external calls and compares ordered events and float bits. It does not validate callees, launch the game or activate a replacement. CI pending.

Next evidence extraction captures the neighboring weapon-FOV calculation, its adjustment helper, following view function, default-FOV selection/update and extra-camera consumer. These are not yet reconstructed. Original defaults unchanged. Store MP only.

## View FOV and projection continuation

Run 45894860739 passed 8192 original CG_GetViewFov comparisons. Reference CG_GetViewFov at 0041FA50 corroborates Store 1400C1B90 identity; the original asserts local client zero, explaining the adjacent global. The reference CG_GetWeaponViewFov and CG_GetTurretViewFov correspond to neighboring 1400C12F0 and 1400C1A00 candidates; their bodies remain to be recovered.

Five more enclosing source-body candidates: 1400F9280 (CG_GetDefaultFovForView), 1400F92F0 (clear one view-mode flag and synchronize), 1400F93E0 (set view-mode byte and synchronize), 1400F9470 (synchronize without changing flags), and 1400C2060 (CG_CalcFov, confirmed reference match). Projection preserves both inlined passes of CG_CalcFovInternal, Store-specific DualPlay clearing, timer reset and wrapping difference, exact tangent multiplier 0x3c0efa35, portrait/aspect branches, seven refdef/tween/sensitivity field stores. It calls the real boundary for GetViewFov and tangent in the original and compares mocked boundary events in tests. FOV default helpers use both mode flags and skip writes for equal or unordered floats. Pending 16384 comparisons. No enhanced changes or live redirect.

## Turret FOV

Store 1400C1A00..1400C1B8B (106 instructions) matches CG_GetTurretViewFov. The incoming FOV lane is unused: original baseline is 55 degrees. Preserve both weapon selection calls, independent weapon property gate, packed attachment selection/first-null termination, original unordered comparisons, mode-dependent transition lookup and exact ADS fraction math. The original LTCG callees at 14065B060 and 14065B120 preserve live XMM1..4; the boundary mocks explicitly preserve those registers. New 16384-case original-PE replay pending. No live runtime activation.

## Weapon and ADS FOV

Store 1400C12F0..1400C19F0 (460 instructions) matches CG_GetWeaponViewFov. Its first ABI lane is confirmed unused. Source recovers the early weapon state 44 interpolation, weapon property gate, cg_useWeaponBasedVariableZoom, two attachment-specific duration scans (first non-sentinel before first null), three zoom values (last non-sentinel before first null), original 100ms maximum frame interval, progress and timer writes, 15-slot weapon zoom-mode lookup, signed remainder, previous zoom blend for elapsed <125ms, and final ADS transition. Candidate test observes original result bits, persistent progress/time and six boundary calls; 32768 cases pending. Some internal LTCG calls preserve volatile registers used by their callers; mocks preserve those lanes explicitly. No native activation yet.

## Extra-camera view continuation

Run 45895409738 passed 32768 complete weapon FOV comparisons. Extra-camera full body candidate at 14016CE80..14016D269 preserves separate extra-camera aspect and FOV overrides, original tangent scale, viewport setup (including unsigned viewport dimensions), scripted camera selection, linked/demo camera dispatch, player view copies, view-height offset and final angle-to-axis boundary. New differential replay compares 30 output words and ordered calls in 16384 cases; pending CI. Next extractor adds main CG_CalcViewValues, extra wrapper, vehicle gunner, surrounding dispatch helpers, angle-to-axis and both ADS transition helpers. No live activation.

## Camera support functions

Run 45895664126 passed all 16384 extra-camera body comparisons. Four new full bodies: CG_GetExtraCamMode 1400C1240 (49 instructions), saved spike camera view 1400C3390 (19), and the two ADS transition accessors 14065B060 and 14065B120 (51 each). Exact packed attachment extraction, first-null termination and original unordered comparison preserved. 24576 original body comparisons queued. The attachment accessors have no external function calls; the extra-mode replay controls missile-active boundary. Main view evidence (538 instructions) now available. No runtime redirect.

## Main view enclosing body

Authored complete 538-instruction CG_CalcViewValues candidate at 14016C510..14016CE7D. Includes adaptive skin LOD hysteresis, three-way blur accumulation, mode update, initial and final FOV projection calls, viewport copy, demo and linked-view early exits, bob/speed, inlined camera-height smoothing, signed time wrapping, prediction error decay, vehicle/turret/first-person dispatch, recoil/tween dispatch, missile angle retention, viewmodel tag animation, third-person dispatch and ADS Z scale. All original addresses are annotated; the table bases are aliases for actual indexed global arrays (not original pointer globals), and the local-client-zero domain must be preserved during binding. The tag object alias is a pointer to a pointer, not a dvar. The two provisional unused/retained ABI lanes need callee-level checking. Source compilation pending; no original differential comparison of this main body yet. The extractor now captures 16 further callees including first/third person and velocity; no claim their bodies are reconstructed.

Main-view differential replay added: 16384 deterministic inputs, 29 original direct call targets plus two imports, 52 compared output words including tag-object flags, and complete ordered call trace. Includes original cubemap/linked early exits and regular dispatch. The original 1400DE970 callee preserves RCX; the mock does likewise. The test models client zero, static globals and callee boundaries, not the actual viewmodel skeleton or rendering. CI pending. Camera support replay passed run 45896536212 (24576 comparisons).

## Camera motion and orientation

Four full source-body candidates: AnglesToAxis 140464D00 (77 instructions), CG_UpdateCameraVelocity 14016C380 (77), CG_PerturbCamera 140157BC0 (73), CG_UpdateCameraTween 1400BF940 (171). Preserve exact original float constants, time wrapping, packed sin/cos return lanes, normalization and matrix boundary, pitch and yaw wrapping, cosine easing, entity-based tween origin/angles and final frame-state copies. The velocity routine scales displacement by elapsed time as observed, rather than substituting a conventional derivative. 16384 original-body comparisons pending; the test substitutes CRT trig and engine matrix/entity boundaries. Main view compile passed 45896635987, independent full-body validation still pending. No runtime redirection.

## First-person and shake

Full source candidates at 14016BA40..14016BD96 and 1400C7540..1400C7713. The first-person body retains the bob input stack structure, original bob/angle boundaries, height, sideways bob, 150/450 ms step smoothing and minimum height. Shake body retains four slots, strongest amplitude/frequency selection, random phase refresh and three independent sine rates. 16384 differential comparisons pending. Camera motion CI 45896899398 passed axes and velocity (8192 comparisons), but failed perturbation on NaN sign propagation. Ordered SSE multiplication/subtraction correction is committed; remaining motion tests pending. No live activation.

## Main camera mode

Full candidate at 1400C0E20..1400C1235, 252 instructions. Preserves demo mode gate, vehicle seat and definition checks, independent entity lookups, turret and missile modes, prior/current transition state comparisons, vehicle exit pitch handling, sound futz selection and original mode-state writes. 32768 original differential comparisons pending. Camera motion NaN correction passed full replay run 45897504478. First-person/shake CI pending. No live redirect.

## Third-person view

Run 45897672565 passed all 49152 first-person/shake/mode comparisons. Complete third-person candidate at 14016A940..14016B04B preserves player/death camera tag resolution, corpse camera horizontal capsule constraint and vertical clamp, chase camera dispatch, original 45-degree pitch ceiling, focus point and collision trace, water-height adjustment, and final angle-to-axis conversion. 32768 original-body comparisons queued; collision trace and tag transform are explicit external boundaries. No runtime redirect.

## Camera detail helpers

Complete bodies at 1400C72B0 (shake attenuation), 140472FE0 (nonlinear view offset), and 1400C3950 (remote eyes tag resolution). Shake temporal/spatial branches, original packed trig lanes, signed time arithmetic and tag fallback/remote mode boundaries retained. 24576 full-function cases pending. Further extraction includes remote special/fallback, camera transition and vehicle updates. No live activation.

## Bob waveform helpers

Run 45898185322 passed third-person and detail helpers (57344 comparisons). New full bodies for vertical bob 1406602A0..14066046C and horizontal bob 140660470..1406605EE, preserving state precedence, raw vec2 dvar lanes, ADS interpolation, speed scaling exception for high-state path, type-aware max dvar and original MINSS behavior, exact waveform constants. 32768 original differential comparisons pending; sine and atof remain external boundaries.

## Third-person collision wrapper

Complete candidate at 1400C69F0..1400C6D28, including ignore-entity precedence, original +/-6 bounds, trace context sentinels, inverse-square-root fallback, threshold interpolation, optional second trace with raised endpoint, and original fraction return. The trace result/context types are provisional byte layouts, not claimed full engine structure definitions. Underlying engine collision query 140190530 is still external. 32768 comparisons pending, with aliased output/start and second-query context persistence.

## Windows-target object audit

The camera replay builds use host C++ with explicit MS x64 call bridges. A separate Win64 COFF audit now compiles all 16 camera source translation units with build_recon.compile_unit and the production x64 flags, checking the machine identifier and every expected public decorated function symbol. The audit supplies only isolated cstdint/cstddef/cstring compatibility adapters (builtin memcpy); it does not change the normal build headers. This is compilation, not a DLL link, rebased binding audit or live replacement. Pending CI.

## Chase camera

Full candidate for CG_OffsetChaseCamView 14016A5D0..14016A930 preserves mode-specific yaw, demo update, original floor-based turn normalization, two packed trig calls, atan2, chase collision mask and original angle-axis boundary. 32768 original comparisons pending. Windows compilation audit initially failed because llvm-nm was missing after successful first object compilation; use available GNU nm fallback (COFF support) without relaxing symbol assertions.

## Orientation and linked view roll

Two additional complete candidates: AxisToAngles 140462D70..140462EBD, and linked roll transformation 140621C10..140621E33. Preserve packed trig call, original special vertical-axis roll branches, sign/quadrant handling, transposed 3x3 row dot products and roll adjustment condition. 32768 original-body comparisons pending. Vector-to-angle and demo update callee evidence added to extraction.

## Vehicle camera offset

Full 180-instruction body at 1400C66D0..1400C69EF preserves six cleared offset fields, seat-specific entity jitter gate, dvar conversion, weapon selection/fallback, random angle perturbation, demo timestamp discontinuities, vehicle type exclusions, shortest-turn pitch smoothing and +/-30-degree correction limit. 32768 full-function cases queued. This is not the separate 1140-instruction vehicle view routine.

## Camera basis and remote motion

Latest prior run 45898978967 passed vehicle offset and all 33 public Win64 functions. Four new complete source candidates: VectorToAngles 140461020 (58 instructions), tag quaternion-to-axis conversion 1400DE970 (70), CG_CalcCubemapViewValues 14016BDA0 (74), remote oscillation 1400C33F0 (129). Cubemap face order checked directly against the Store jump table, tag scale comes from the original record rather than imposing normalization, remote motion reads two sets of raw dvar lanes and preserves speed window and waveform order. 32768 comparisons and updated Win64 audit pending. The remote boundary in the earlier remote-eye body is not yet replaced. No live activation.

## Turret view positioning

Complete candidate at 14016BFB0..14016C374. Includes entity and model map resolution, player tag, missing tag error, weapon first-person offset, original random angular jitter, history reset outside 300 ms window, view offset helper and local-axis translation. Original engine helper 1400C23C0 is an explicit call boundary; extractor captures it next. 16384 original-body comparisons pending. No live activation.
