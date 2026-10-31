#include "prx/libwebrtc/libwebrtc.hpp"
#include <array>
#include <barrier>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {

constexpr auto Current = _ZN6webrtc13TaskQueueBase7CurrentEv_nid_postfix;
constexpr auto Enter = _ZN6webrtc13TaskQueueBase22CurrentTaskQueueSetterC1EPS0__nid_postfix;
constexpr auto Leave = _ZN6webrtc13TaskQueueBase22CurrentTaskQueueSetterD1Ev_nid_postfix;

void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "WebRTC task queue context: %s\n", message);
        std::abort();
    }
}

struct QueueScope {
    WebRtcCurrentTaskQueueSetter setter;

    explicit QueueScope(WebRtcTaskQueue* queue) { Enter(&setter, queue); }
    ~QueueScope() { Leave(&setter); }
    QueueScope(const QueueScope&) = delete;
    QueueScope& operator=(const QueueScope&) = delete;
};

void Nesting() {
    std::array<std::uint64_t, 2> tokens{};
    auto* first = reinterpret_cast<WebRtcTaskQueue*>(&tokens[0]);
    auto* second = reinterpret_cast<WebRtcTaskQueue*>(&tokens[1]);
    Require(Current() == nullptr, "initial queue is null");
    {
        QueueScope outer(first);
        Require(Current() == first && outer.setter.previous == nullptr, "outer queue is installed");
        {
            QueueScope inner(second);
            Require(Current() == second && inner.setter.previous == first, "nested queue saves outer queue");
            {
                QueueScope cleared(nullptr);
                Require(Current() == nullptr && cleared.setter.previous == second, "null queue temporarily clears context");
                QueueScope sameNull(nullptr);
                Require(Current() == nullptr && sameNull.setter.previous == nullptr, "nested null scope");
            }
            Require(Current() == second, "restore after clearing context");
            {
                QueueScope same(second);
                Require(Current() == second && same.setter.previous == second, "same-queue nested scope");
            }
            Require(Current() == second, "restore same-queue scope");
        }
        Require(Current() == first, "nested destruction restores outer queue");
    }
    Require(Current() == nullptr, "outer destruction restores null");
}

void LayoutAndUnwinding() {
    std::uint64_t token{};
    auto* queue = reinterpret_cast<WebRtcTaskQueue*>(&token);
    QueueScope outer(queue);
    struct GuardedSetter {
        std::uint64_t before;
        WebRtcCurrentTaskQueueSetter setter;
        std::uint64_t after;
    } guarded{0x0123456789abcdef, {}, 0xfedcba9876543210};
    Enter(&guarded.setter, nullptr);
    Require(guarded.setter.previous == queue, "constructor saves full-width pointer");
    Require(guarded.before == 0x0123456789abcdef && guarded.after == 0xfedcba9876543210, "constructor only writes the one-pointer object");
    Leave(&guarded.setter);
    Require(Current() == queue && guarded.setter.previous == queue, "destructor retains saved field");
    Require(guarded.before == 0x0123456789abcdef && guarded.after == 0xfedcba9876543210, "destructor preserves guards");
    try {
        QueueScope inner(nullptr);
        throw 42;
    } catch (int value) {
        Require(value == 42 && Current() == queue, "exception unwinding restores queue");
    }
    bool enterThrew = false, leaveThrew = false;
    try { Enter(nullptr, nullptr); } catch (const std::invalid_argument&) { enterThrew = true; }
    try { Leave(nullptr); } catch (const std::invalid_argument&) { leaveThrew = true; }
    Require(enterThrew && leaveThrew && Current() == queue, "invalid objects throw without changing context");
}

void ThreadIsolation() {
    constexpr int workerCount = 8;
    std::barrier rendezvous(workerCount + 1);
    std::uint64_t mainToken{};
    auto* mainQueue = reinterpret_cast<WebRtcTaskQueue*>(&mainToken);
    QueueScope mainScope(mainQueue);
    std::vector<std::thread> workers;
    for (int worker = 0; worker < workerCount; ++worker) {
        workers.emplace_back([&] {
            std::uint64_t token{};
            auto* queue = reinterpret_cast<WebRtcTaskQueue*>(&token);
            Require(Current() == nullptr, "new thread starts with no queue");
            {
                QueueScope scope(queue);
                rendezvous.arrive_and_wait();
                Require(Current() == queue, "concurrent queues stay thread-local");
                {
                    QueueScope cleared(nullptr);
                    Require(Current() == nullptr, "worker clears only its own context");
                }
                Require(Current() == queue, "worker restores only its own context");
                rendezvous.arrive_and_wait();
            }
            Require(Current() == nullptr, "worker leaves no active queue");
        });
    }
    rendezvous.arrive_and_wait();
    Require(Current() == mainQueue, "workers do not replace main queue");
    rendezvous.arrive_and_wait();
    for (auto& worker : workers) worker.join();
    Require(Current() == mainQueue, "worker exit leaves main queue intact");
}

}

int main() {
    Nesting();
    LayoutAndUnwinding();
    Require(Current() == nullptr, "unwinding fixture restores initial state");
    ThreadIsolation();
    Require(Current() == nullptr, "thread fixture restores initial state");
}
