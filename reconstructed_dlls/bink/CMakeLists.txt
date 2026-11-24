cmake_minimum_required(VERSION 3.20)
project(t6_native_bink LANGUAGES CXX)
find_package(Threads REQUIRED)
add_library(t6_native_bink STATIC
    huffman_codebook.cpp
    entropy_byte_rle.cpp
    entropy_adaptive_pairs.cpp
    entropy_nibbles.cpp
    entropy_signed_bytes.cpp
    entropy_deltas16.cpp
    entropy_bytes.cpp
    video_coefficients.cpp
    video_inverse8x8.cpp
    video_inverse8x8_2x.cpp
    video_inverse8x8_indexed.cpp
    video_inverse8x8_2x_indexed.cpp
    video_block_modes.cpp
    video_predict8x8_core.cpp
    video_predict8x8.cpp
    video_sparse_residual.cpp
    video_luma_first_keyframe.cpp
    header_validation.cpp
    bink_index.cpp
    resident_frame_state.cpp
    resident_bootstrap.cpp
    file_io_state.cpp
    bink_allocator.cpp
    resident_video_session.cpp
    bink_dispatch.cpp
    bink_leaf.cpp
    bink_small.cpp
    bink_frames.cpp
    async_lifecycle.cpp
    bink_native_api.cpp
)
target_compile_features(t6_native_bink PUBLIC cxx_std_20)
target_link_libraries(t6_native_bink PUBLIC Threads::Threads)
target_include_directories(t6_native_bink PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
add_executable(test_native_api test_native_api.cpp)
target_link_libraries(test_native_api PRIVATE t6_native_bink)
add_executable(test_native_session test_resident_video_session.cpp)
target_link_libraries(test_native_session PRIVATE t6_native_bink)
add_executable(test_native_pool test_native_pool.cpp)
target_link_libraries(test_native_pool PRIVATE t6_native_bink)
foreach(t test_native_api test_native_session test_native_pool)
    if(MSVC)
        target_compile_options(${t} PRIVATE /UNDEBUG)
    else()
        target_compile_options(${t} PRIVATE -UNDEBUG)
    endif()
endforeach()

option(T6_BINK_TRACE_BITS "Diagnostic first-plane bitstream trace" OFF)
if(T6_BINK_TRACE_BITS)
    target_compile_definitions(t6_native_bink PRIVATE T6_BINK_TRACE_BITS=1)
    add_executable(test_native_plane_trace test_native_plane_trace.cpp)
    target_link_libraries(test_native_plane_trace PRIVATE t6_native_bink)
endif()

# Optional real-game caller validation only. The final rebuilt EXE
# links t6_native_bink directly; this target is not the final dependency-free build.
option(T6_BINK_STORE_VALIDATION "Build source-only Store import-compatible validation DLL" OFF)
if(T6_BINK_STORE_VALIDATION)
    if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "Store validation target requires Windows x64")
    endif()
    add_library(t6_bink_store_validation SHARED store_validation.cpp store_bink_exports.def)
    target_link_libraries(t6_bink_store_validation PRIVATE t6_native_bink)
    set_target_properties(t6_bink_store_validation PROPERTIES OUTPUT_NAME binkw64
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/store-validation"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/store-validation")
    add_executable(test_store_import_client test_native_api.cpp)
    # Test client resolves all Bink names through the new import library.
    target_link_libraries(test_store_import_client PRIVATE t6_bink_store_validation)
    target_compile_features(test_store_import_client PRIVATE cxx_std_20)
    target_compile_options(test_store_import_client PRIVATE /UNDEBUG)
    set_target_properties(test_store_import_client PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/store-validation")
endif()
