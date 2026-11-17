#include "gnss_processor.h"

#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n\f\v");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n\f\v");
    return value.substr(first, last - first + 1);
}

bool is_unsigned_integer(const std::string& value) {
    if (value.empty()) return false;
    for (unsigned char ch : value) {
        if (!std::isdigit(ch)) return false;
    }
    return true;
}

bool is_signed_integer(const std::string& value) {
    if (value.empty()) return false;
    std::size_t i = (value[0] == '+' || value[0] == '-') ? 1 : 0;
    if (i == value.size()) return false;
    for (; i < value.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(value[i]))) return false;
    }
    return true;
}

bool is_decimal(const std::string& value) {
    std::size_t i = 0;
    if (i < value.size() && (value[i] == '+' || value[i] == '-')) ++i;
    bool has_digits = false;
    while (i < value.size() && std::isdigit(static_cast<unsigned char>(value[i]))) {
        ++i;
        has_digits = true;
    }
    if (i < value.size() && value[i] == '.') {
        ++i;
        while (i < value.size() && std::isdigit(static_cast<unsigned char>(value[i]))) {
            ++i;
            has_digits = true;
        }
    }
    if (!has_digits) return false;
    if (i < value.size() && (value[i] == 'e' || value[i] == 'E')) {
        ++i;
        if (i < value.size() && (value[i] == '+' || value[i] == '-')) ++i;
        const auto exponent_start = i;
        while (i < value.size() && std::isdigit(static_cast<unsigned char>(value[i]))) ++i;
        if (i == exponent_start) return false;
    }
    return i == value.size();
}

bool parse_row(const std::string& line, GnssFrame& frame) {
    std::istringstream row(line);
    std::vector<std::string> fields;
    std::string cell;
    while (std::getline(row, cell, ',')) fields.push_back(cell);
    if (fields.size() != 6) return false;
    for (auto& field : fields) field = trim(field);

    // Enforce the documented field types before conversion. In particular,
    // textual coordinates (for example "lon=as") are malformed CSV rows.
    if (!is_unsigned_integer(fields[0]) ||
        !is_decimal(fields[1]) || !is_decimal(fields[2]) || !is_decimal(fields[3]) ||
        !is_signed_integer(fields[4]) || !is_signed_integer(fields[5])) return false;

    try {
        std::size_t consumed = 0;
        frame.timestamp_ms = std::stoull(fields[0], &consumed);
        if (consumed != fields[0].size()) return false;

        consumed = 0;
        frame.lat = std::stod(fields[1], &consumed);
        if (consumed != fields[1].size()) return false;

        consumed = 0;
        frame.lon = std::stod(fields[2], &consumed);
        if (consumed != fields[2].size()) return false;

        consumed = 0;
        frame.hdop = std::stof(fields[3], &consumed);
        if (consumed != fields[3].size()) return false;

        consumed = 0;
        frame.satellites_used = std::stoi(fields[4], &consumed);
        if (consumed != fields[4].size()) return false;

        consumed = 0;
        frame.fix_type = std::stoi(fields[5], &consumed);
        return consumed == fields[5].size();
    } catch (const std::exception&) {
        return false;
    }
}
}  // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: replay_gnss <csv-path>\n";
        return 2;
    }

    std::ifstream input(argv[1]);
    if (!input) {
        std::cerr << "Cannot open CSV: " << argv[1] << '\n';
        return 2;
    }

    std::string line;
    if (!std::getline(input, line)) {
        std::cerr << "Missing CSV header\n";
        return 2;
    }
    line = trim(line);  // Accept either LF or CRLF files on Windows and Unix.
    if (line != "timestamp_ms,lat,lon,hdop,satellites_used,fix_type") {
        std::cerr << "Invalid CSV header\n";
        return 2;
    }

    GnssProcessor processor;
    std::size_t rows = 0;
    std::size_t accepted = 0;
    std::size_t rejected = 0;
    while (std::getline(input, line)) {
        ++rows;
        GnssFrame frame{};
        if (!parse_row(line, frame)) {
            ++rejected;
            std::cerr << "Rejected malformed CSV row " << rows + 1 << '\n';
            continue;
        }
        if (processor.process(frame)) ++accepted;
        else ++rejected;
    }

    if (input.bad()) {
        std::cerr << "Error reading CSV input\n";
        return 2;
    }
    std::cout << "Replay summary: rows=" << rows << ", accepted=" << accepted
              << ", rejected=" << rejected << '\n';
    return 0;
}