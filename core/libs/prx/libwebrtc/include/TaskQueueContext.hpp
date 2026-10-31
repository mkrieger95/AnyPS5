#pragma once

struct WebRtcTaskQueue;

struct WebRtcCurrentTaskQueueSetter {
    WebRtcTaskQueue* previous;
};

static_assert(sizeof(WebRtcCurrentTaskQueueSetter) == 8);
