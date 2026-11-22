// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "jewel/core.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <map>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

namespace jewel {

class GlauberGeometry {
    struct Entry { double b, cumulative; };
    std::vector<Entry> entries_;
    double mean_overlap_ = 0;
    double overlap_se_ = 0;

    static double number(const std::string& column) {
        std::size_t end = 0;
        const double value = std::stod(column, &end);
        if (end != column.size() || !std::isfinite(value))
            throw std::invalid_argument("invalid Glauber CSV number");
        return value;
    }

public:
    GlauberGeometry(const std::filesystem::path& path, double low, double high) {
        std::ifstream in(path);
        std::string line;
        if (!std::getline(in, line) ||
            line != "trial,b_fm,npart,ncoll,taa_mb_inverse,centrality_low_pct,centrality_high_pct")
            throw std::invalid_argument("expected a glauber-pbpb events.csv");
        double cumulative = 0, sum = 0, square_sum = 0, previous_b = -1, previous_class = -1, sigma = 0;
        std::map<unsigned, std::size_t> class_sizes;
        std::unordered_set<unsigned> trials;
        unsigned bin_width = 0;
        double support = 0;
        while (std::getline(in, line)) {
            std::istringstream row(line);
            std::array<double, 7> values{};
            std::string field;
            for (auto& value : values) {
                if (!std::getline(row, field, ','))
                    throw std::invalid_argument("incomplete Glauber CSV record");
                value = number(field);
            }
            if (row.peek() != std::char_traits<char>::eof() || line.back() == ',')
                throw std::invalid_argument("extra Glauber CSV column");
            const auto [trial, b, npart, ncoll, taa, lo, hi] = values;
            if (trial < 1 || trial > 10000000 || trial != std::floor(trial)
                || !trials.insert(static_cast<unsigned>(trial)).second ||
                b < 0 || b < previous_b || b > 20 || npart < 2 || npart > 416
                || npart != std::floor(npart) || ncoll < 1 || ncoll > 208*208
                || ncoll != std::floor(ncoll) || npart > 2*ncoll || taa <= 0
                || lo < 0 || lo < previous_class || lo >= hi || hi > 100 || lo != std::floor(lo) || hi != std::floor(hi))
                throw std::invalid_argument("invalid or unordered Glauber event");
            const auto width = static_cast<unsigned>(hi-lo);
            if (!bin_width) bin_width = width;
            if (!sigma) sigma = ncoll/taa;
            if (width != bin_width || 100 % width || static_cast<unsigned>(lo) % width
                || std::abs((ncoll/taa)/sigma-1) > 1e-9)
                throw std::invalid_argument("inconsistent Glauber bins or nucleon cross section");
            previous_b = b;
            previous_class = lo;
            ++class_sizes[static_cast<unsigned>(lo)];
            if (lo < high && hi > low) {
                if (lo < low || hi > high)
                    throw std::invalid_argument("centrality edges must align with full Glauber bins");
                support = b;
                cumulative += ncoll;
                entries_.push_back({b, cumulative});
                sum += taa;
                square_sum += taa*taa;
            }
        }
        if (!in.eof() || entries_.size() < 2 || class_sizes.empty() || class_sizes.size()*bin_width != 100)
            throw std::invalid_argument("Glauber input must contain a complete minimum-bias event table");
        // The runner samples vertices from a rectangle with half-width 10 fm.
        if (support >= 20)
            throw std::invalid_argument("Glauber impact parameter exceeds vertex support");
        const auto total = trials.size();
        for (const auto& [edge, count] : class_sizes)
            if (count != total*(edge+bin_width)/100 - total*edge/100)
                throw std::invalid_argument("Glauber centrality bin counts do not match minimum-bias ranks");
        const double n = entries_.size();
        mean_overlap_ = sum/n;
        overlap_se_ = std::sqrt(std::max(0., square_sum-sum*sum/n)/(n*(n-1)));
    }

    double sample(Random& random) const {
        const double total = entries_.back().cumulative;
        const double target = std::min(random.uniform()*total, std::nextafter(total, 0.));
        return std::upper_bound(entries_.begin(), entries_.end(), target,
                                [](double x, const Entry& entry) { return x < entry.cumulative; })->b;
    }

    std::size_t size() const { return entries_.size(); }
    double mean_overlap() const { return mean_overlap_; }
    double overlap_se() const { return overlap_se_; }
};

} // namespace jewel
