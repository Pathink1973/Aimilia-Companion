#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>

struct PlaybackDiagnosticCounts {
    uint32_t decoded = 0;
    uint32_t played = 0;
    uint32_t audible = 0;
};

// Scalar counters only. Never log or store microphone/speech content.
class PlaybackDiagnostics {
public:
    void Decoded() { decoded_.fetch_add(1, std::memory_order_relaxed); }
    void Played(const int16_t* pcm, size_t size) {
        bool audible = false;
        for (size_t i = 0; i < size; i += 8) {
            if (pcm[i] > 128 || pcm[i] < -128) {
                audible = true;
                break;
            }
        }
        if (audible)
            audible_.fetch_add(1, std::memory_order_relaxed);
        played_.fetch_add(1, std::memory_order_relaxed);
    }
    PlaybackDiagnosticCounts Get() const {
        return {decoded_.load(std::memory_order_relaxed), played_.load(std::memory_order_relaxed),
                audible_.load(std::memory_order_relaxed)};
    }

private:
    std::atomic<uint32_t> decoded_{0}, played_{0}, audible_{0};
};
