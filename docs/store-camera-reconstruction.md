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
