#!/usr/bin/env python3
"""Compile recovered Store HUD/font functions to Win64 COFF, not a live link."""
import pathlib, shutil, subprocess, tempfile
from verify_store_camera_win64_objects import ROOT,build_recon,HEADERS
UNITS={"src/ui/lui_text_render_store.cpp":["DrawText"],
       "src/ui/lui_inline_icon_store.cpp":["DrawInlineIcon"],
       "src/ui/ui_text_wrappers_store.cpp":["DrawTextScaled","DrawTextRotated","DrawTextNormalized","ItemFont","DecimalSeparator","DrawTextGlow","DrawHudText","DrawDistance"],
       "src/qcommon/localize_bindings_store.cpp":["Lookup","BindingText","ReplaceBinding"],
       "src/gfx_d3d/r_matrix_display_store.cpp":["UpdateMatrixBase","GetMatrixConstant","MonitorRect","SelectDesktopResolution","SetFullscreenOutput","BeginRendererSync","EndRendererSync"],
       "src/gfx_d3d/r_display_resources_store.cpp":["SetUI3DViewport","BuildSwapChainDesc","UpdateVideoConfig","SetWindowMode","ResetClientQueries","CreateQueryPairs","InitDynamicBuffer","AllocateResourceSlot","RegisterTargetName","ResetTextureCache"],
       "src/ui/lui_text_util_store.cpp":["DecodeCharacter","ColorForCode","TransformPoint","RectVertices","FontHeight","AnimatedY","SubmitQuadChain","ReleaseQuadChain"],
       "src/gfx_d3d/r_material_target_store.cpp":["ApplyMaterialState","SetRenderTarget","ApplyPassArgs"],
       "src/gfx_d3d/r_text_command_variants_store.cpp":["AddCmdEndOfList","AddCmdUI3DMarker","AddCmdDrawTextEffects","AddCmdDrawTextDecode","AddCmdDrawTextRedact","AddCmdStretchPic","AddCmdStretchPicRotate","AddCmdUIQuadArray"],
       "src/gfx_d3d/rb_text_dispatch_store.cpp":["DrawText2DCmd","DrawText3DCmd"],
       "src/gfx_d3d/rb_text_loop_store.cpp":["DrawTeleType","DrawText2D"],
       "src/gfx_d3d/rb_font_flush_store.cpp":["EndTessSurface"],
       "src/ui/ui_font_support_store.cpp":["RegisterMaterial","SafeTranslate","DrawTextField"],
       "src/gfx_d3d/r_font_gpu_state_store.cpp":["UploadIndices","UploadVertices","DrawIndexed","BindVertexShader","SetPass","SetCodeConstant","SetStableArgs","SetMaterialArgs"],
       "src/gfx_d3d/rb_font_world_store.cpp":["DrawTextInSpace"],
       "src/gfx_d3d/r_font_state_store.cpp":["GetViewport","SetViewportConstants","SetWorldTextTransform","SetVertexBuffer"],
       "src/gfx_d3d/rb_font_geometry_store.cpp":["BeginSurface","StretchPic","StretchPicRotate","GlyphQuad3D"],
       "src/gfx_d3d/rb_font_effects_store.cpp":["PrintLength","DrawEffectSlice","DrawNamedIcon","DrawLegacyIcon"],
       "src/gfx_d3d/r_font_aux_store.cpp":["WrapPosition","RingTextWidth","ReplaceText","DrawBorder"],
       "src/gfx_d3d/rb_font_store.cpp":["DrawCursor","LookupIcon","ConvertColor"],
       "src/gfx_d3d/r_text_commands_store.cpp":["AddBaseDrawTextCmd","AddCmdDrawQuad"],
       "src/ui/ui_font_draw_store.cpp":["BlinkText","DrawTextPadding"],
       "src/gfx_d3d/r_fonticon_store.cpp":["GetFontIconEntryInfo"],
       "src/gfx_d3d/r_font_store.cpp":["DefaultFont","FontHandle","CharacterGlyph","RegisterFont","TextWidth"],
       "src/ui/ui_font_assets_store.cpp":["AssetCache","ResolutionScale"],
       "src/client/screen_placement_store.cpp":["HudViewX","HudViewY","VirtualFromRealWidth","VirtualFromRealHeight","ApplyRect"]}
def main():
    with tempfile.TemporaryDirectory() as tmp:
        tmp=pathlib.Path(tmp);headers=tmp/"headers";headers.mkdir()
        adapters=dict(HEADERS)
        adapters["cstdint"]+="\nnamespace std { using ::uint16_t; }\n"
        for name,body in adapters.items():(headers/name).write_text(body)
        total=0
        for path,funcs in UNITS.items():
            obj=tmp/(pathlib.Path(path).stem+".obj")
            build_recon.compile_unit(ROOT/path,obj,"x64",["-DT6_STORE","-DT6_STORE_FONT_REPLAY","-isystem"+str(headers),"-ffunction-sections"])
            assert obj.read_bytes()[:2]==b"\x64\x86"
            syms=subprocess.check_output([shutil.which("llvm-nm") or shutil.which("nm"),"--defined-only",str(obj)],text=True)
            for f in funcs:assert "?"+f+"@" in syms,(path,f)
            total+=len(funcs)
            print(path,len(funcs),"Win64 public bodies")
        print(total,"Store font functions compiled; external references not linked.")
if __name__=="__main__":main()
