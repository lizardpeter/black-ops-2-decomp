# Store menu build number and UI registration

The reference build has separate UI_DrawBuildNumber and UI_RegisterDvars
functions. The Store compiler inlined the draw path in UI_Refresh and
the boolean registration in UI_RegisterDvars. The latter is now fully
authored in src/ui/ui_register_dvars.cpp.

| Build | UI_RegisterDvars | UI_Refresh | ui_drawBuildNumber pointer |
| --- | --- | --- | --- |
| MP | 1403F9B20 | 1403FD090 | 142121320 |
| ZM | 1403FA4F0 | 1403FDA60 | 14211F3A0 |

UI_RegisterDvars has a complete 8,143-byte original body in both
executables. The C++ reconstruction includes the 83 registration
operations (54 inline lookup/new-or-reregister paths and 29 wrapper
calls), 53 stored dvar pointers and both additional registration
callee calls, including the final tail call. The original default
of ui_drawBuildNumber is true. The shared authored body takes a bool
so the future enhanced replacement can register false without
changing the remaining registration sequence.

    python tests/verify_store_ui_registration.py /path/to/store/files

Executed successfully with the two SHA-256-pinned Store PEs: 256
cases per build, 512 complete executions. Each compares 139 ordered
events and 53 global pointer values (98,304 event/pointer comparisons).
The seed selects all new, all existing or mixed inline lookup results;
both defaults are compared against the original body, with just the
default immediate changed in the isolated mapping for the false cases.
The nine registration callee boundaries are mocked. The reconstructed
source compiles to Windows x64 COFF in isolation. The related and tail
registration functions are not themselves reconstructed. No whole
Windows runtime build, live UI test or campaign validation is claimed.
UI_Refresh is not yet fully authored, and the existing P0017 loader
patch remains the active enhanced implementation until the source
replacement can be validated in a full runtime build. No standard
differential status promotion is made.
