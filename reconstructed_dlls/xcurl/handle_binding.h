#pragma once
#include "api_binding.h"
#include "easy_lifecycle.h"
#include "multi_lifecycle.h"
#include "handle_lifecycle.h"
namespace t6_xcurl {
// Composition of original public handle wrappers, their constructors and
// destructors. External owner must serialize all global lifecycle calls and
// ensure detached/drained easy handles before destruction.
class HandleBinding {
public:
    HandleBinding(ApiBinding&,const EasyLifecycleServices&,const MultiLifecycleServices&);
    HandleBinding(const HandleBinding&)=delete;
    HandleBinding& operator=(const HandleBinding&)=delete;
    void* easy_init();
    void easy_cleanup(void*);
    void* multi_init();
    std::uint32_t multi_cleanup(void*);
private:
    ApiBinding& api_;
    const EasyLifecycleServices& easy_;
    const MultiLifecycleServices& multi_;
    HandleServices handles_;
    void record(std::uint32_t);
    void error(std::uint32_t,std::uint32_t);
    static HandleBinding& self(void*);
    static void* allocate(std::size_t,std::uint32_t,void*);
    static void release(void*,std::uint32_t,void*);
    static void initialize_lock(void*,void*);
    static std::uint32_t start_global(void*);
    static std::uint32_t start_easy(void*,void*);
    static void destroy_easy(void*,void*);
    static std::uint32_t start_multi(void*,void*);
    static std::uint32_t destroy_multi(void*,void*);
};
}
