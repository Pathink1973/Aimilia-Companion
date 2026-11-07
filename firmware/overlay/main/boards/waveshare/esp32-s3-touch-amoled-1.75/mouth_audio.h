#pragma once
#include <atomic>
#include <algorithm>
#include <cstdint>
#include <cstddef>

// Only scalar telemetry crosses audio/UI tasks; no display calls on audio thread.
namespace mouth_audio {
inline std::atomic<unsigned> level{0};
inline std::atomic<uint32_t> updated_ms{0};
inline unsigned Measure(const int16_t* pcm, size_t size) {
    uint64_t sum = 0;
    size_t count = 0;
    for (size_t i = 0; i < size; i += 4) {
        int sample = pcm[i];
        sum += sample < 0 ? -sample : sample;
        ++count;
    }
    return count ? static_cast<unsigned>(std::min<uint64_t>(100, sum / count / 35)) : 0;
}
inline unsigned Read(uint32_t now) {
    return now - updated_ms.load(std::memory_order_relaxed) < 200
        ? level.load(std::memory_order_relaxed) : 0;
}
}
