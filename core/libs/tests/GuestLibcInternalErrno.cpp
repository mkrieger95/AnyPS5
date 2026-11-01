#include "prx/libSceLibcInternal/libSceLibcInternal.hpp"
#include "prx/libc/include/Api.hpp"

#include <array>
#include <atomic>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <thread>

namespace {

void Require(bool condition, const char* message) {
    if (condition) return;
    std::fprintf(stderr, "libc internal errno: %s\n", message);
    std::abort();
}

void CheckStorage() {
    int* error = _Geterrno_nid_postfix();
    Require(error != nullptr && error == __error_nid_postfix(), "the getter must share guest libc's errno storage");
    for (const int value : {0, 1, 14, 22, -123, INT_MIN, INT_MAX}) {
        *__error_nid_postfix() = value;
        Require(_Geterrno_nid_postfix() == error && *error == value, "the getter must preserve the existing error and its address");
        *_Geterrno_nid_postfix() = ~value;
        Require(*__error_nid_postfix() == ~value, "writes through the getter must be visible to guest libc");
    }
    *error = 0;
    Require(access_nid_postfix(nullptr, 0) == -1 && *_Geterrno_nid_postfix() == 14, "a guest libc failure must be visible through the getter");
}

void CheckThreadStorage() {
    std::array<int*, 2> errors{};
    std::array<std::thread, 2> threads;
    std::atomic<unsigned> ready{0};
    std::atomic<unsigned> written{0};
    std::atomic<bool> start{false};
    std::atomic<bool> finish{false};
    for (std::size_t index = 0; index < threads.size(); ++index) {
        threads[index] = std::thread([&, index] {
            errors[index] = _Geterrno_nid_postfix();
            Require(errors[index] != nullptr && errors[index] == __error_nid_postfix(), "a worker must share its guest libc errno storage");
            ready.fetch_add(1, std::memory_order_release);
            while (!start.load(std::memory_order_acquire)) {}
            *errors[index] = static_cast<int>(100 + index);
            Require(_Geterrno_nid_postfix() == errors[index] && *__error_nid_postfix() == static_cast<int>(100 + index), "a worker's errno must remain stable");
            written.fetch_add(1, std::memory_order_release);
            while (!finish.load(std::memory_order_acquire)) {}
        });
    }
    while (ready.load(std::memory_order_acquire) != threads.size()) {}
    int* mainError = _Geterrno_nid_postfix();
    Require(errors[0] != mainError && errors[1] != mainError && errors[0] != errors[1], "simultaneously live threads must have distinct errno storage");
    *mainError = 321;
    start.store(true, std::memory_order_release);
    while (written.load(std::memory_order_acquire) != threads.size()) {}
    Require(*mainError == 321 && *errors[0] == 100 && *errors[1] == 101, "a worker must not overwrite another thread's errno");
    finish.store(true, std::memory_order_release);
    for (auto& thread : threads) thread.join();
}

}

int main() {
    const int previous = *__error_nid_postfix();
    CheckStorage();
    CheckThreadStorage();
    *__error_nid_postfix() = previous;
    return 0;
}
