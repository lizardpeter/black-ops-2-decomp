# Store MP HUD resolution investigation

Confirmed from the previously reconstructed Store MP renderer registration at 14070E230:
- r_fontResolution (pointer 1427E71D0) defaults to 720, domain 720..1080, flags 0x40.
- r_resolution (pointer 1427E6F88) defaults to 720, domain 480..1080, flags 1. Do not conflate this with the actual DXGI display dimensions.
- ui_smallFont and ui_bigFont are scale thresholds, not texture resolution.

Reference x86 evidence: R_GetFontPathFromName (00A2AD60) formats fonts/%d/%s using r_fontResolution. The DB registry formats _%d%s for CODE_FAST_FILE_RES_NAME. UI_AssetCache (0070F...; see named function in reference pseudocode) registers the normal, bold, small, big, console, extra-small and extra-big font assets through that path. This suggests resolution-tiered assets, not a proof of which fastfiles the Store ships or chooses.

The new Store HUD evidence workflow scans the pinned MP executable for exact RIP references to the two resolution dvar pointer slots, HUD scale, capture scaling and relevant literal strings; it extracts enclosing function bodies. Direct references only; inlined/indirect alias analysis and asset inventory are separate. The capture flag 1427F1B60 is not evidence of a normal fixed HUD render target; it is used in the independently recovered capture path. No production changes. Do not extend the font dvar domain until assets and lookup behavior have been verified. No screenshot or live render capture yet.

## Store evidence and first faithful source batch

Run 45901118115 found 218 exact RIP references and 60 enclosing bodies. UI_AssetCache 1403FBAF0 registers tiered fonts and two Store UI materials (ui_blur, ui_line_graph). DB naming 140215300 formats _%d%s at 14021574D. Crucially 1407203B0 calls the integer dvar setter 14046CDF0 with source 0 and literal 720 at 140720506, then calls 140215300: the Store renderer initialization explicitly requests 720 regardless of display size. This is an exact call-site observation; other writes/overrides and runtime values require separate verification.

Six complete source candidates (501 original instructions): UI_AssetCache 1403FBAF0 (402), font resolution scale 1400D0050 (31), HudViewX 1400A9AB0 (21), HudViewY 1400A9B20 (21), VirtualFromRealHeight 1400D7110 (13), VirtualFromRealWidth 1400DDB40 (13). Preserve the original special-language shared font aliases and normal material registration order, type-aware dvar reads, signed placement index fallback, viewport and capture scaling. New focused replay requests 49,152 comparisons and Win64 COFF audit. These are not live bindings. No enhanced changes.

## Core glyph / measurement / selection

Run 45901652018 passed all 8192 UI_AssetCache cases, then failed resolution scale: Store helper returns 504 / resolution (720 * 0.7), not 720 / resolution. Corrected candidate; verify numerator and null default against the actual PE constants before claiming match.
Run 45901628819 extracted 33 bodies, including direct glyph lookup 14071A6E0 (46 instructions), font registration 14071A780 (5), R_TextWidth 14071A7A0 (168), UI_GetFontHandleDefault 1403FCD20 (77), UI_GetFontHandle 1403FCE40 (109). Five source candidates in src/gfx_d3d/r_font_store.cpp. Preserve direct ASCII glyph indexing, binary search from index 96, fallback index 14, unsigned advance byte, UTF-8 decoder, longest line measurement, formatting escapes and font icon sizing. R_TextWidth does not apply kerning itself. Font selection uses exact threshold comparisons including unordered float behavior and twice-applied HUD scale. No enhanced edits.

## Draw dispatch and inline icons

Run 45901945491 passed 49,152 original-PE comparisons and the six-function Win64 COFF audit. Added three more complete source candidates: BlinkText 1403FF700 (68 instructions), UI_DrawTextPadding 1403FC6D0 (126), R_GetFontIconEntryInfo 140723450 (139). These preserve language-10 early return, one-time timer initialization, 800..999 ms special glyph substitution, original 1024-byte text buffer and replace loop, normalized scale, rounded physical coordinates, draw-command parameters, case-insensitive icon hashes, ordered alias table searches, and default material handling. Callees remain explicit. These three are not yet validated. The text-command allocator at 14071C880 and placement rect 1401F65E0 are distinct remaining bodies. No live redirects or enhanced modifications.
