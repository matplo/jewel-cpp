// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <array>
#include <cstddef>
#include <vector>
namespace jewel {
// Transient generator-record boundary, not the persistent stable-ID shower graph.
// Slots are zero-based; slot zero is the reserved scratch record.
struct CompressionRecord {
 std::array<int,5> k{};
 std::array<double,5> p{},v{},production{};
 int colour{},anticolour{};
 double fraction{},angle{},evolution_time{};
 bool quark_pair{},angular_required{},medium_induced{};
};
struct CompressionResult {
 std::size_t active{},selected{},removed{};
 bool room_left{}; // Stock returns true only when more than ten records were removed.
 std::vector<std::size_t> source_slots; // Retained source slots, including scratch.
};
// selected==active means no protected record. The vector retains its
// original physical size and the inactive tail, matching the source routine.
// Moved genealogy is cleared; evolution_time and medium_induced stay in slots.
CompressionResult compress_records(std::vector<CompressionRecord>&,std::size_t active,std::size_t selected);
inline CompressionResult compress_records(std::vector<CompressionRecord>& records,std::size_t selected){
 return compress_records(records,records.size(),selected);
}
}
