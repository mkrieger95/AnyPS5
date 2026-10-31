#include "prx/libwebrtc/libwebrtc.hpp"
#include "prx/libc/include/HostThreadLocal.hpp"
#include <stdexcept>

namespace {

struct CurrentTaskQueueTag {};

}

extern "C" {

WebRtcTaskQueue* APS5_VABI _ZN6webrtc13TaskQueueBase7CurrentEv_nid_postfix(void) {
    return HostThreadLocal<WebRtcTaskQueue*, CurrentTaskQueueTag>();
}

void APS5_VABI _ZN6webrtc13TaskQueueBase22CurrentTaskQueueSetterC1EPS0__nid_postfix(WebRtcCurrentTaskQueueSetter* setter, WebRtcTaskQueue* queue) {
    if (!setter) throw std::invalid_argument("CurrentTaskQueueSetter: null object");
    auto& current = HostThreadLocal<WebRtcTaskQueue*, CurrentTaskQueueTag>();
    setter->previous = current;
    current = queue;
}

void APS5_VABI _ZN6webrtc13TaskQueueBase22CurrentTaskQueueSetterD1Ev_nid_postfix(WebRtcCurrentTaskQueueSetter* setter) {
    if (!setter) throw std::invalid_argument("CurrentTaskQueueSetter destructor: null object");
    HostThreadLocal<WebRtcTaskQueue*, CurrentTaskQueueTag>() = setter->previous;
}

}
