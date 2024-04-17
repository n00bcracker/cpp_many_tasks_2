#include "context.h"

#include "cactus/core/debug.h"

#include <vector>
#include <cstdint>

#include <glog/logging.h>

#ifdef __has_feature
#define SANITIZE_ADDRESS __has_feature(address_sanitizer)
#elif defined(__SANITIZE_ADDRESS__)
#define SANITIZE_ADDRESS 1
#else
#define SANITIZE_ADDRESS 0
#endif

namespace __cxxabiv1 {                          // NOLINT
typedef void __untyped_cxa_exception;           // NOLINT
struct __cxa_eh_globals {                       // NOLINT
    __untyped_cxa_exception* CaughtExceptions;  // NOLINT
    unsigned int UncaughtExceptions;            // NOLINT
};
extern "C" __cxa_eh_globals* __cxa_get_globals() noexcept;  // NOLINT
}  // namespace __cxxabiv1

extern "C" cactus::MachineContext* CactusContextActivate(cactus::MachineContext* ctx,
                                                         cactus::SwitchArgs* data);
extern "C" void CactusContextTrampoline();
extern "C" void CactusContextEntry(cactus::MachineContext* saved_ctx) noexcept;

#if SANITIZE_ADDRESS
extern "C" void __sanitizer_start_switch_fiber(void** fake_stack_save, const void* bottom,
                                               size_t size);
extern "C" void __sanitizer_finish_switch_fiber(void* fake_stack_save, void** bottom_old,
                                                size_t* size_old);
#endif

namespace cactus {

struct SwitchArgs {
    SavedContext* from = nullptr;
    SavedContext* to = nullptr;
};

SavedContext::SavedContext() = default;

SavedContext::SavedContext(char* stack, size_t stack_size) {
    auto stack_top = stack + stack_size;
    DCHECK(reinterpret_cast<int64_t>(stack_top) % 16 == 0);

    // Push extra qword, that way rsp+8 aligns on 16 bytes after trampoline.
    *reinterpret_cast<uint64_t*>(stack_top - 8) = 0;

    // Create fake frame on the stack.
    rsp = reinterpret_cast<MachineContext*>(stack_top - sizeof(MachineContext) - 8);
    *rsp = {};

    // First activation works as if fiber was suspended just before calling CactusContextTrampoline.
    rsp->saved_rip = reinterpret_cast<void*>(&CactusContextTrampoline);

    sanitizer_stack_bottom = stack;
    sanitizer_stack_size = stack_size;
}

// Cache pointer in TLS for faster access.
thread_local __cxxabiv1::__cxa_eh_globals* eh_globals = __cxxabiv1::__cxa_get_globals();

static_assert(sizeof(__cxxabiv1::__cxa_eh_globals) == SavedContext::kErasedSize);

void SavedContext::Activate(SavedContext* suspend_to) {
    *reinterpret_cast<__cxxabiv1::__cxa_eh_globals*>(eh.data()) = *eh_globals;
    *eh_globals = {};

    SwitchArgs args;
    args.from = suspend_to;
    args.to = this;
    DCHECK(suspend_to != this);

    auto ctx = rsp;
    rsp = nullptr;
    DCHECK(ctx);

#if SANITIZE_ADDRESS
    __sanitizer_start_switch_fiber(&suspend_to->sanitizer_fake_stack, sanitizer_stack_bottom,
                                   sanitizer_stack_size);
#endif

    auto save_point = CactusContextActivate(ctx, &args);
    auto switch_args = reinterpret_cast<SwitchArgs*>(save_point->data);
    if (switch_args->from) {
        switch_args->from->rsp = save_point;
    }
    DCHECK(switch_args->to == suspend_to);

#if SANITIZE_ADDRESS
    if (switch_args->from) {
        __sanitizer_finish_switch_fiber(suspend_to->sanitizer_fake_stack,
                                        &switch_args->from->sanitizer_stack_bottom,
                                        &switch_args->from->sanitizer_stack_size);
    } else {
        __sanitizer_finish_switch_fiber(suspend_to->sanitizer_fake_stack, nullptr, nullptr);
    }
#endif

    *eh_globals = *reinterpret_cast<__cxxabiv1::__cxa_eh_globals*>(eh.data());
    eh = {};
}

void CactusContextEntry(MachineContext* parent) {
    auto args = reinterpret_cast<SwitchArgs*>(parent->data);
    args->from->rsp = parent;

#if SANITIZE_ADDRESS
    __sanitizer_finish_switch_fiber(nullptr, &args->from->sanitizer_stack_bottom,
                                    &args->from->sanitizer_stack_size);
#endif

    auto entry = args->to->entry;
    args->to->entry = nullptr;
    DCHECK(entry);

    auto last = entry->Run();

    *eh_globals = {};

    auto ctx = last->rsp;
    last->rsp = nullptr;
    DCHECK(ctx);

    SwitchArgs last_args;
    last_args.from = nullptr;
    last_args.to = last;

#if SANITIZE_ADDRESS
    __sanitizer_start_switch_fiber(nullptr, last->sanitizer_stack_bottom,
                                   last->sanitizer_stack_size);
#endif

    CactusContextActivate(ctx, &last_args);
}

}  // namespace cactus

extern "C" void CactusContextEntry(cactus::MachineContext* parent) noexcept {
    cactus::CactusContextEntry(parent);

    // Should never return.
    std::terminate();
}
