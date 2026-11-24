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
    bink_native_api.cpp
)
target_compile_features(t6_native_bink PUBLIC cxx_std_20)
target_link_libraries(t6_native_bink PUBLIC Threads::Threads)
target_include_directories(t6_native_bink PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
add_executable(test_native_api test_native_api.cpp)
target_link_libraries(test_native_api PRIVATE t6_native_bink)
add_executable(test_native_session test_resident_video_session.cpp)
target_link_libraries(test_native_session PRIVATE t6_native_bink)
foreach(t test_native_api test_native_session)
    if(MSVC)
        target_compile_options(${t} PRIVATE /UNDEBUG)
    else()
        target_compile_options(${t} PRIVATE -UNDEBUG)
    endif()
endforeach()
