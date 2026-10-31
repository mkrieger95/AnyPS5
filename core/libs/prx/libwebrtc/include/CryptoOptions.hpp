#pragma once

#include <cstddef>
#include <type_traits>

struct WebRtcCryptoOptions {
    struct Srtp {
        bool enableGcmCryptoSuites;
        bool enableAes128Sha1_32CryptoCipher;
        bool enableAes128Sha1_80CryptoCipher;
        bool enableEncryptedRtpHeaderExtensions;
    } srtp;

    struct SFrame {
        bool requireFrameEncryption;
    } sframe;
};

static_assert(sizeof(bool) == 1);
static_assert(std::is_standard_layout_v<WebRtcCryptoOptions>);
static_assert(sizeof(WebRtcCryptoOptions::Srtp) == 4);
static_assert(offsetof(WebRtcCryptoOptions::Srtp, enableGcmCryptoSuites) == 0);
static_assert(offsetof(WebRtcCryptoOptions::Srtp, enableAes128Sha1_32CryptoCipher) == 1);
static_assert(offsetof(WebRtcCryptoOptions::Srtp, enableAes128Sha1_80CryptoCipher) == 2);
static_assert(offsetof(WebRtcCryptoOptions::Srtp, enableEncryptedRtpHeaderExtensions) == 3);
static_assert(offsetof(WebRtcCryptoOptions, sframe) == 4);
static_assert(sizeof(WebRtcCryptoOptions) == 5);
