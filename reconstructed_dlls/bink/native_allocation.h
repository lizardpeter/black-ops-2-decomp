#pragma once
#include "bink_allocator.h"
#include <cstdlib>
#include <cstring>
#include <new>
#include <stdexcept>

namespace t6_bink {
// Original 32-byte-aligned allocation footer, with host CRT only as
// the default platform heap boundary. Free follows the saved callback.
class NativeAllocation {
    void* memory_=nullptr;
    Allocate alloc_=nullptr;
    Deallocate free_=nullptr;
public:
    NativeAllocation()=default;
    ~NativeAllocation(){clear();}
    NativeAllocation(const NativeAllocation&)=delete;
    NativeAllocation& operator=(const NativeAllocation&)=delete;
    void configure(Allocate a,Deallocate f) {
        if(memory_ || bool(a)!=bool(f))throw std::invalid_argument("invalid allocator pair");
        alloc_=a;free_=f;
    }
    void resize(std::size_t words) {
        clear();
        if(words>UINT32_MAX/8U)throw std::bad_alloc();
        if(!words)return;
        memory_=bink_research_allocate_with_fallback(static_cast<std::uint32_t>(words*8),
                                                     alloc_,free_,std::malloc);
        if(!memory_)throw std::bad_alloc();
        std::memset(memory_,0,words*8);
    }
    void clear() {
        if(memory_)bink_research_free_with_fallback(memory_,std::free);
        memory_=nullptr;
    }
    bool empty() const {return memory_==nullptr;}
    std::uint64_t* data(){return static_cast<std::uint64_t*>(memory_);}
    const std::uint64_t* data()const{return static_cast<const std::uint64_t*>(memory_);}
};
}
