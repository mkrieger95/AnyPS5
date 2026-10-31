#include "prx/libwebrtc/libwebrtc.hpp"
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

namespace {

constexpr auto Copy = _ZN6webrtc13CryptoOptionsC1ERKS0__nid_postfix;

void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "WebRTC crypto options: %s\n", message);
        std::abort();
    }
}

WebRtcCryptoOptions Options(unsigned bits) {
    return {{bool(bits & 1), bool(bits & 2), bool(bits & 4), bool(bits & 8)}, {bool(bits & 16)}};
}

unsigned Bits(const WebRtcCryptoOptions& options) {
    return unsigned(options.srtp.enableGcmCryptoSuites) |
        (unsigned(options.srtp.enableAes128Sha1_32CryptoCipher) << 1) |
        (unsigned(options.srtp.enableAes128Sha1_80CryptoCipher) << 2) |
        (unsigned(options.srtp.enableEncryptedRtpHeaderExtensions) << 3) |
        (unsigned(options.sframe.requireFrameEncryption) << 4);
}

struct GuardedOptions {
    std::array<unsigned char, 8> before;
    WebRtcCryptoOptions options;
    std::array<unsigned char, 8> after;
};

static_assert(offsetof(GuardedOptions, after) == offsetof(GuardedOptions, options) + 5);

void CheckCopies() {
    GuardedOptions destination;
    destination.before.fill(0xa5);
    destination.after.fill(0x5a);
    for (unsigned bits = 0; bits < 32; ++bits) {
        GuardedOptions source;
        source.before.fill(0x12);
        source.after.fill(0x34);
        source.options = Options(bits);
        destination.options = Options(bits ^ 31);
        const auto originalSource = source;
        Copy(&destination.options, &source.options);
        Require(Bits(destination.options) == bits, "copy every combination of the five flags");
        Require(std::memcmp(&source, &originalSource, sizeof(source)) == 0, "copy leaves the entire source unchanged");
        for (unsigned char byte : destination.before) Require(byte == 0xa5, "preserve bytes before destination");
        for (unsigned char byte : destination.after) Require(byte == 0x5a, "preserve bytes after five-byte destination");
        source.options = Options(bits ^ 31);
        Require(Bits(destination.options) == bits, "destination owns an independent value");
    }
    std::array<WebRtcCryptoOptions, 2> adjacent{Options(21), Options(10)};
    Copy(&adjacent[1], &adjacent[0]);
    Require(Bits(adjacent[1]) == 21 && Bits(adjacent[0]) == 21, "adjacent objects may be copied");
    adjacent[0] = Options(6);
    Copy(&adjacent[0], &adjacent[1]);
    Require(Bits(adjacent[0]) == 21, "copy in the opposite address direction");
}

template <typename TCall>
void RequireThrows(TCall call) {
    try { call(); }
    catch (const std::invalid_argument&) { return; }
    Require(false, "invalid construction must throw");
}

}

int main() {
    CheckCopies();
    WebRtcCryptoOptions source = Options(21), destination = Options(10);
    RequireThrows([&] { Copy(nullptr, &source); });
    RequireThrows([&] { Copy(&destination, nullptr); });
    RequireThrows([] { Copy(nullptr, nullptr); });
    RequireThrows([&] { Copy(&source, &source); });
    Require(Bits(source) == 21 && Bits(destination) == 10, "rejected copies leave objects unchanged");
    std::array<unsigned char, 12> overlap{};
    overlap.fill(0x5a);
    const auto original = overlap;
    for (std::size_t offset = 1; offset < sizeof(WebRtcCryptoOptions); ++offset) {
        auto* first = reinterpret_cast<WebRtcCryptoOptions*>(overlap.data());
        auto* second = reinterpret_cast<WebRtcCryptoOptions*>(overlap.data() + offset);
        RequireThrows([&] { Copy(first, second); });
        RequireThrows([&] { Copy(second, first); });
    }
    Require(overlap == original, "reject partial overlap before reading or writing flags");
}
