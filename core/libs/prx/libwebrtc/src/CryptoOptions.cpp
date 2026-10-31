#include "prx/libwebrtc/libwebrtc.hpp"
#include <cstdint>
#include <stdexcept>

extern "C" {

void APS5_VABI _ZN6webrtc13CryptoOptionsC1ERKS0__nid_postfix(WebRtcCryptoOptions* destination, const WebRtcCryptoOptions* source) {
    if (!destination || !source) throw std::invalid_argument("CryptoOptions copy: null object");
    const auto destinationAddress = reinterpret_cast<std::uintptr_t>(destination);
    const auto sourceAddress = reinterpret_cast<std::uintptr_t>(source);
    const auto distance = destinationAddress < sourceAddress ? sourceAddress - destinationAddress : destinationAddress - sourceAddress;
    if (distance < sizeof(WebRtcCryptoOptions)) throw std::invalid_argument("CryptoOptions copy: overlapping objects");
    *destination = *source;
}

}
