#pragma once
#include "bink_index.h"
#include "resident_frame_state.h"
#include "video_registered.h"
#include <array>
#include <string>
#include <vector>

namespace t6_bink {
// Research integration of the original-derived resident frame-state
// routines and registered video coordinator. The minimal state bootstrap
// is a deliberate source adapter, NOT a recovered BinkOpen initializer.
// All file and table storage is owned here. Registered output is borrowed;
// its caller must keep the descriptor and both frame slots alive.
// This is synchronous, trusted Store BIKi video only, no original DLL ABI,
// streaming, audio, worker threads or OS timer implementation.
class ResidentVideoSession {
public:
    enum class DecodeResult { frame, already_decoded, error };
    ResidentVideoSession()=default;
    ResidentVideoSession(const ResidentVideoSession&)=delete;
    ResidentVideoSession& operator=(const ResidentVideoSession&)=delete;
    ResidentVideoSession(ResidentVideoSession&&)=delete;
    ResidentVideoSession& operator=(ResidentVideoSession&&)=delete;
    bool open(std::vector<std::uint8_t> file,ResidentTimer timer,void* ctx);
    void close();
    bool register_buffers(RegisteredFrameBuffers& info);
    DecodeResult do_frame();
    bool next_frame();
    bool pause(std::uint32_t value);
    const HeaderPrefix& header() const {return header_;}
    std::uint32_t frame_number() const;
    std::uint32_t last_decoded_frame() const;
    std::uint32_t keyframe() const;
    bool is_paused() const;
    const std::string& error() const {return error_;}
    unsigned plane_count() const {return header_.flags&kAlphaFlag?4U:3U;}
    unsigned plane_width(unsigned p) const;
    unsigned plane_height(unsigned p) const;
private:
    bool fail(std::string reason);
    HeaderPrefix header_{};
    std::vector<std::uint8_t> file_;
    std::vector<std::uint32_t> index_;
    alignas(8) std::array<std::uint8_t,0x5A8> handle_{};
    std::array<std::array<std::uint32_t,32>,7> history_{};
    RegisteredFrameBuffers* info_=nullptr;
    ResidentTimer timer_=nullptr;
    void* clock_ctx_=nullptr;
    bool opened_=false,failed_=false;
    std::string error_;
};
}
