cmake_minimum_required(VERSION 3.20)
project(t6_native_xcurl_research LANGUAGES CXX)
# Only the already reconstructed leaf and lifecycle source. Not a full
# curl ABI or a networking implementation; never substitute it for XCurl.dll.
add_library(t6_native_xcurl_research STATIC
    allocation.cpp duplicate_string.cpp slist.cpp easy_strerror.cpp
    getinfo_unlocked.cpp api_profile.cpp global_lifecycle.cpp)
add_library(t6::xcurl_research ALIAS t6_native_xcurl_research)
target_compile_features(t6_native_xcurl_research PUBLIC cxx_std_20)
target_include_directories(t6_native_xcurl_research PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
option(T6_XCURL_BUILD_TESTS "Build source-link smoke test" ON)
if(T6_XCURL_BUILD_TESTS)
    enable_testing()
    add_executable(test_xcurl_native_link test_native_link.cpp)
    target_link_libraries(test_xcurl_native_link PRIVATE t6::xcurl_research)
    if(MSVC)
        target_compile_options(test_xcurl_native_link PRIVATE /UNDEBUG)
    else()
        target_compile_options(test_xcurl_native_link PRIVATE -UNDEBUG)
    endif()
    add_test(NAME xcurl_native_link COMMAND test_xcurl_native_link)
endif()
