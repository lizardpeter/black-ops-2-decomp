// Opt-in Store MP renderer registration replacement. All policy/defaults live
// in the faithful src body; the enhanced hook only changes which code runs.
#if defined(T6_STORE) && defined(T6_STORE_RENDERER_NATIVE)
namespace store_renderer_dvar { void RegisterRendererComplete(); }
extern "C" void Store_Renderer_RegisterOriginal() {
    store_renderer_dvar::RegisterRendererComplete();
}
#endif
