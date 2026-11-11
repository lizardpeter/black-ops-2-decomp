cmake_minimum_required(VERSION 3.16)
project(t6_reconstruction LANGUAGES C)

set(T6_FAITHFUL_SOURCES
    src/universal/com_math.c
    src/universal/com_shared.c
    src/universal/q_shared.c
)

add_library(t6_faithful SHARED ${T6_FAITHFUL_SOURCES})
include(enhanced/sources.cmake)
add_library(t6_enhanced SHARED ${T6_ENHANCED_SOURCES})

foreach(target IN ITEMS t6_faithful t6_enhanced)
    target_include_directories(${target} PUBLIC include)
    target_compile_features(${target} PRIVATE c_std_11)
    set_target_properties(${target} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endforeach()
