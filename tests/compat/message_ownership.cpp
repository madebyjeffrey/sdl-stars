#include "internal.h"
#include <stars_compat.h>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <thread>

#define CHECK(f) do { if (!(f)) { std::fprintf(stderr,"line %d: %s (%s)\n",__LINE__,#f,SDL_GetError()); return 1; } } while (0)
static std::atomic<bool> fFailCalloc, fFailMalloc;
void *StarsTest_calloc(size_t c, size_t cb) noexcept {
    if (fFailCalloc.exchange(false)) return nullptr;
    return std::calloc(c,cb);
}
void *StarsTest_malloc(size_t cb) noexcept {
    if (fFailMalloc.exchange(false)) return nullptr;
    return std::malloc(cb);
}
static std::atomic<int> cReleased;
static int cDelivered, md;
static bool fCallbackOk = true;
static constexpr UINT wmPayload = WM_APP + 70, wmNested = WM_APP + 71;
static STARSCOMPATPAYLOAD MakePayload(int value) {
    auto *pv = static_cast<int *>(std::malloc(sizeof(int)));
    if (pv) *pv = value;
    return STARSCOMPATPAYLOAD(pv, {[](void *p) noexcept { ++cReleased; std::free(p); }});
}
static LRESULT CALLBACK Proc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    if (msg != wmPayload && msg != wmNested) return DefWindowProcA(hwnd, msg, w, l);
    auto *value = static_cast<int *>(PvCompatMessagePayload(w));
    if (!value) return 0; // Repeated/stale copies have no owned payload.
    ++cDelivered;
    if (msg == wmNested) { fCallbackOk &= *value == 22; return 0; }
    fCallbackOk &= *value == 11;
    if (md == 1) {
        fCallbackOk &= StarsCompatPostOwnedMessageAsync(StarsCompatWindowId(hwnd),wmNested,MakePayload(22),0);
        MSG nested{};
        fCallbackOk &= PeekMessageA(&nested,hwnd,wmNested,wmNested,PM_REMOVE);
        DispatchMessageA(&nested);
        fCallbackOk &= PvCompatMessagePayload(w) == value && *value == 11;
    } else if (md == 2) {
        DestroyWindow(hwnd);
        fCallbackOk &= *value == 11; // Dispatch still owns it after HWND teardown.
    } else if (md == 3) {
        throw StarsError{starsErrorFile,33};
    }
    return 0;
}
static HWND MakeWindow() {
    WNDCLASSA wc{}; wc.lpfnWndProc = Proc; wc.lpszClassName = "OwnedMessageProbe";
    RegisterClassA(&wc);
    return CreateWindowExA(0,wc.lpszClassName,"payload",WS_OVERLAPPEDWINDOW,0,0,160,120,nullptr,nullptr,nullptr,nullptr);
}
static bool Take(HWND hwnd, MSG &msg) { return PeekMessageA(&msg,hwnd,wmPayload,wmNested,PM_REMOVE); }
int main() {
    CHECK(FCompatVideo());
    HWND hwnd = MakeWindow(); CHECK(hwnd);
    uint64_t id = StarsCompatWindowId(hwnd); CHECK(id);
    MSG msg{};
    int before = cReleased;
    CHECK(StarsCompatPostOwnedMessageAsync(id,wmPayload,MakePayload(11),0));
    CHECK(Take(hwnd,msg) && !PvCompatMessagePayload(msg.wParam));
    DispatchMessageA(&msg); CHECK(cReleased == before+1 && cDelivered == 1);
    DispatchMessageA(&msg); CHECK(cReleased == before+1 && cDelivered == 1);

    before = cReleased; md = 1;
    CHECK(StarsCompatPostOwnedMessageAsync(id,wmPayload,MakePayload(11),0));
    CHECK(Take(hwnd,msg)); DispatchMessageA(&msg);
    CHECK(cReleased == before+2 && cDelivered == 3 && fCallbackOk);

    before = cReleased; md = 3;
    CHECK(StarsCompatPostOwnedMessageAsync(id,wmPayload,MakePayload(11),0));
    CHECK(Take(hwnd,msg)); DispatchMessageA(&msg);
    CHECK(cReleased == before+1 && FStarsErrorPending());
    try { StarsRethrowPendingError(); CHECK(false); }
    catch (const StarsError &error) { CHECK(error.ids == 33); }

    before = cReleased; md = 0;
    CHECK(!StarsCompatPostOwnedMessageAsync(UINT64_MAX,wmPayload,MakePayload(11),0));
    CHECK(cReleased == before+1);
    before = cReleased;
    fFailCalloc = true;
    CHECK(!StarsCompatPostOwnedMessageAsync(id,wmPayload,MakePayload(11),0));
    CHECK(!fFailCalloc && cReleased == before+1);
    before = cReleased;
    fFailMalloc = true;
    bool fAllocationPosted = true;
    std::thread allocationFailure([&] { fAllocationPosted = StarsCompatPostOwnedMessageAsync(id,wmPayload,MakePayload(11),0); });
    allocationFailure.join();
    CHECK(!fAllocationPosted && !fFailMalloc && cReleased == before+1);
    bool fPosted = false;
    before = cReleased;
    std::thread worker([&] { fPosted = StarsCompatPostOwnedMessageAsync(id,wmPayload,MakePayload(11),0); });
    worker.join(); CHECK(fPosted && Take(hwnd,msg));
    DispatchMessageA(&msg); CHECK(cReleased == before+1);

    before = cReleased;
    SDL_SetEventFilter([](void *, SDL_Event *event) -> bool { return event->type != SDL_EVENT_USER; },nullptr);
    std::thread rejected([&] { fPosted = StarsCompatPostOwnedMessageAsync(id,wmPayload,MakePayload(11),0); });
    rejected.join(); SDL_SetEventFilter(nullptr,nullptr);
    CHECK(!fPosted && cReleased == before+1);

    // A producer retains the record while SDL runs foreign event filters.
    struct BARRIER {
        std::atomic<bool> fEntered{false}, fRelease{false};
        SDL_Event event{};
    } barrier;
    SDL_SetEventFilter([](void *pv, SDL_Event *event) -> bool {
        if (event->type == SDL_EVENT_USER && event->user.code == COMPAT_ASYNC_POST_CODE) {
            auto &b = *static_cast<BARRIER *>(pv);
            b.event = *event;
            b.fEntered.store(true,std::memory_order_release);
            while (!b.fRelease.load(std::memory_order_acquire)) std::this_thread::yield();
        }
        return true;
    },&barrier);
    before = cReleased;
    std::thread racing([&] { fPosted = StarsCompatPostOwnedMessageAsync(id,wmPayload,MakePayload(11),0); });
    const uint64_t deadline = SDL_GetTicks()+2000;
    while (!barrier.fEntered.load(std::memory_order_acquire) && SDL_GetTicks()<deadline) std::this_thread::yield();
    const bool fEntered = barrier.fEntered.load(std::memory_order_acquire);
    if (fEntered) CompatStopAsyncMessages();
    const bool fRetained = cReleased == before;
    barrier.fRelease.store(true,std::memory_order_release);
    racing.join(); SDL_SetEventFilter(nullptr,nullptr);
    CHECK(fEntered && fRetained && cReleased == before+1);
    CompatConsumeAsyncMessage(&barrier.event); // Already canceled; no old dereference.
    CompatConsumeAsyncMessage(&barrier.event); // Repeated stale event is harmless.
    CHECK(cReleased == before+1);
    CompatStartAsyncMessages();

    before = cReleased;
    CHECK(StarsCompatPostOwnedMessageAsync(id,wmPayload,MakePayload(11),0));
    CHECK(Take(hwnd,msg)); // Removed from queue, deliberately never dispatched.
    CHECK(DestroyWindow(hwnd)); CHECK(cReleased == before+1);
    DispatchMessageA(&msg); CHECK(cReleased == before+1);
    hwnd = MakeWindow(); CHECK(hwnd); id = StarsCompatWindowId(hwnd);
    before = cReleased; md = 2;
    CHECK(StarsCompatPostOwnedMessageAsync(id,wmPayload,MakePayload(11),0));
    CHECK(Take(hwnd,msg)); DispatchMessageA(&msg);
    CHECK(cReleased == before+1 && fCallbackOk && !IsWindow(hwnd));

    hwnd = MakeWindow(); CHECK(hwnd); id = StarsCompatWindowId(hwnd); md = 0;
    before = cReleased;
    CHECK(StarsCompatPostOwnedMessageAsync(id,wmPayload,MakePayload(11),0));
    CHECK(Take(hwnd,msg));
    CHECK(StarsCompatPostOwnedMessageAsync(id,wmPayload,MakePayload(11),0));
    std::thread unpumped([&] { fPosted = StarsCompatPostOwnedMessageAsync(id,wmPayload,MakePayload(11),0); });
    unpumped.join(); CHECK(fPosted);
    StarsCompatProcessExit(); CHECK(cReleased == before+3);
    before = cReleased;
    std::thread late([&] { fPosted = StarsCompatPostOwnedMessageAsync(id,wmPayload,MakePayload(11),0); });
    late.join(); CHECK(!fPosted && cReleased == before+1);

    CHECK(FCompatVideo()); hwnd = MakeWindow(); CHECK(hwnd && StarsCompatWindowId(hwnd) != id);
    before = cReleased; const int cDeliveredBefore = cDelivered;
    std::thread oldTarget([&] { fPosted = StarsCompatPostOwnedMessageAsync(id,wmPayload,MakePayload(11),0); });
    oldTarget.join(); CHECK(fPosted);
    CHECK(!Take(hwnd,msg));
    CHECK(cReleased == before+1 && cDelivered == cDeliveredBefore);
    StarsCompatProcessExit(); StarsCompatProcessExit(); SDL_Quit();
    CHECK(fCallbackOk);
    std::puts("Owned message delivery, nesting, failure, rejection, cancellation, shutdown and stale target tests passed");
    return 0;
}
