// Windows import-compatible validation target for the 19 Store imports.
// All function bodies come from the same native static library. No vendor
// DLL forwarding, no external decoder, no runtime patching of game files.
// The .def supplies the public export roots to the COFF linker.
// The final game rebuild uses the static target, not this DLL.
#include "bink_native_api.h"
static_assert(sizeof(void*)==8);
