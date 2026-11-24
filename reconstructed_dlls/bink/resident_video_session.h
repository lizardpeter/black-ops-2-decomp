#pragma once
#include "bink_index.h"
#include "native_allocation.h"
#include "resident_frame_state.h"
#include "video_registered.h"
#include <array>
#include <memory>
#include <string>
#include <vector>

namespace t6_bink {
// Research integration of the original-derived resident frame-state
// routines and registered video coordinator. The resident external-buffer
// bootstrap layout and observed handle fields are original-derived, but
// the full BinkOpen initializer and OS object lifetime remain unresolved.
// All file and table storage is owned here. Registered output is borrowed;
// its caller must keep the descriptor and both frame slots alive.
// Trusted resident Store BIKi video only, with synchronous decode and a
// single actual host worker thread using the original async dispatch/wait
// state routines. The portable thread is a platform adapter, not the full
// recovered original worker pool or DLL ABI. No streaming or audio.
class ResidentVideoSession {
public:
    enum class DecodeResult { frame, already_decoded, busy, error };
    enum class SubmitResult { submitted, busy, already_decoded, error };
    enum class WaitResult { frame, pending, idle, error };
    ResidentVideoSession();
    ~ResidentVideoSession();
    ResidentVideoSession(const ResidentVideoSession&)=delete;
    ResidentVideoSession& operator=(const ResidentVideoSession&)=delete;
    ResidentVideoSession(ResidentVideoSession&&)=delete;
    ResidentVideoSession& operator=(ResidentVideoSession&&)=delete;
    bool open(std::vector<std::uint8_t> file,ResidentTimer timer,void* ctx);
    bool open_borrowed(const std::uint8_t* file,std::size_t bytes,ResidentTimer timer,void* ctx);
    void set_memory_callbacks(Allocate alloc,Deallocate free_fn);
    // Original fully preloaded filename path; refuse small-buffer streaming
    // rather than pretending that it is resident. Opens read-only.
    bool open_preloaded_file(const char* path,std::uint32_t io_size,ResidentTimer timer,void* ctx);
    // Bounded-memory filename stream, with an actual background refill
    // thread and the original circular-buffer read/seek primitives.
    bool open_streaming_file(const char* path,std::uint32_t io_size,ResidentTimer timer,void* ctx);
    void close();
    void* native_handle() {return opened_?handle_.data():nullptr;}
    std::uint32_t control_background_io(std::uint32_t flags);
    bool register_buffers(RegisteredFrameBuffers& info);
    DecodeResult do_frame();
    // Same-worker submission: original work message tag 6. The host
    // worker and registered buffers stay alive until wait or close.
    SubmitResult do_frame_async(std::uint32_t worker=0);
    WaitResult wait_async(std::int32_t timeout);
    bool async_pending() const;
    bool next_frame();
    bool pause(std::uint32_t value);
    // Valid only after at least one completed frame; does not poison playback
    // on a rejected history window.
    bool realtime(RealtimeSnapshot& output,std::uint32_t frames);
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
    struct Worker;
    struct StreamInput;
    bool fail(std::string reason);
    bool open_impl(std::vector<std::uint8_t> file,ResidentTimer timer,void* ctx,
                   bool file_mode,std::uint32_t io_size);
    bool decode_current_packet(RegisteredDecodeReport& report,std::string& error) const;
    HeaderPrefix header_{};
    std::vector<std::uint8_t> file_;
    NativeAllocation packet_storage_;
    const std::uint8_t* borrowed_=nullptr;
    Allocate alloc_=nullptr;
    Deallocate free_=nullptr;
    std::size_t file_size_=0,packet_offset_=0;
    // Original cache-line-separated bootstrap allocation. Eight-byte
    // aligned backing storage; no original OS mutex ownership here.
    NativeAllocation handle_;
    RegisteredFrameBuffers* info_=nullptr;
    ResidentTimer timer_=nullptr;
    void* clock_ctx_=nullptr;
    bool opened_=false,failed_=false;
    std::string error_;
    std::unique_ptr<Worker> worker_;
    std::unique_ptr<StreamInput> stream_;
};
}
