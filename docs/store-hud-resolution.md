# Store MP HUD resolution investigation

Confirmed from the previously reconstructed Store MP renderer registration at 14070E230:
- r_fontResolution (pointer 1427E71D0) defaults to 720, domain 720..1080, flags 0x40.
- r_resolution (pointer 1427E6F88) defaults to 720, domain 480..1080, flags 1. Do not conflate this with the actual DXGI display dimensions.
- ui_smallFont and ui_bigFont are scale thresholds, not texture resolution.

Reference x86 evidence: R_GetFontPathFromName (00A2AD60) formats fonts/%d/%s using r_fontResolution. The DB registry formats _%d%s for CODE_FAST_FILE_RES_NAME. UI_AssetCache (0070F...; see named function in reference pseudocode) registers the normal, bold, small, big, console, extra-small and extra-big font assets through that path. This suggests resolution-tiered assets, not a proof of which fastfiles the Store ships or chooses.

The new Store HUD evidence workflow scans the pinned MP executable for exact RIP references to the two resolution dvar pointer slots, HUD scale, capture scaling and relevant literal strings; it extracts enclosing function bodies. Direct references only; inlined/indirect alias analysis and asset inventory are separate. The capture flag 1427F1B60 is not evidence of a normal fixed HUD render target; it is used in the independently recovered capture path. No production changes. Do not extend the font dvar domain until assets and lookup behavior have been verified. No screenshot or live render capture yet.
