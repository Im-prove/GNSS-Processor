#pragma once
#include <cstdint>
#include <mutex>

struct GnssFrame {
    uint64_t timestamp_ms;
    double lat;
    double lon;
    float hdop;
    int satellites_used;
    int fix_type;
};

class GnssProcessor {
public:
    struct Config {
        float max_hdop = 5.0f;
        int min_satellites = 4;
        uint64_t max_gap_ms = 1500;
        bool reject_null_island = true; // Set false for marine/other operating areas.
    };

    GnssProcessor();
    explicit GnssProcessor(Config config);

    // Accepts valid frames, prints their position, and returns true. Rejected
    // frames advance received-frame timing when newer, but are never printed.
    bool process(const GnssFrame& frame);
    // Clears timestamp history so a new stream/receiver session can start.
    void reset();

private:
    bool validate(const GnssFrame& frame) const;
    Config config_;
    std::mutex mutex_;
    bool has_received_frame_ = false;
    uint64_t last_received_timestamp_ms_ = 0;
};
