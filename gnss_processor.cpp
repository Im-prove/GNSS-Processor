#include "gnss_processor.h"
#include <cmath>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>

namespace {
// Per-instance locks do not serialize writes from different processors.
std::mutex output_mutex;
}

GnssProcessor::GnssProcessor() : GnssProcessor(Config{}) {}

GnssProcessor::GnssProcessor(Config config) : config_(config) {}

bool GnssProcessor::validate(const GnssFrame& frame) const {
    // Enabled by default for land robots; marine users can disable it in Config.
    return std::isfinite(frame.lat) && frame.lat >= -90.0 && frame.lat <= 90.0 &&
           std::isfinite(frame.lon) && frame.lon >= -180.0 && frame.lon <= 180.0 &&
           (!config_.reject_null_island || !(frame.lat == 0.0 && frame.lon == 0.0)) &&
           std::isfinite(frame.hdop) && frame.hdop >= 0.0f && frame.hdop <= config_.max_hdop &&
           frame.satellites_used >= config_.min_satellites &&
           (frame.fix_type == 1 || frame.fix_type == 2);
}

void GnssProcessor::reset() {
    std::lock_guard<std::mutex> state_lock(mutex_);
    has_received_frame_ = false;
    last_received_timestamp_ms_ = 0;
}

bool GnssProcessor::process(const GnssFrame& frame) {
    std::lock_guard<std::mutex> state_lock(mutex_);
    if (has_received_frame_ && frame.timestamp_ms <= last_received_timestamp_ms_) {
        return false;
    }
    const bool timestamp_gap = has_received_frame_ &&
        frame.timestamp_ms - last_received_timestamp_ms_ > config_.max_gap_ms;
    // Timestamp ordering describes the input stream, so remember every newer
    // packet even when its quality is bad or its gap is excessive.
    last_received_timestamp_ms_ = frame.timestamp_ms;
    has_received_frame_ = true;
    if (timestamp_gap) return false;

    if (!validate(frame)) return false;
    std::ostringstream output;
    output << std::fixed << std::setprecision(6)
           << "Accepted GNSS position: lat=" << frame.lat << ", lon=" << frame.lon << '\n';
    {
        std::lock_guard<std::mutex> output_lock(output_mutex);
        std::cout << output.str();
    }
    return true;
}
