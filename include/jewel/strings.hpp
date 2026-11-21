// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/compression.hpp"
#include "jewel/branching.hpp"
namespace jewel {
struct StringBuildResult {
 std::size_t active{};
 bool discarded{};
 // Original zero-based input slots, retained separately from CLEANUP's erased parents.
 std::vector<std::size_t> output_sources,unconsumed_sources;
};
// Literal MAKESTRINGS_VAC + CLEANUP boundary transformation, not hadronization.
// The vector is expanded to the source's 23000 physical slots. CLEANUP moves
// K/P/V only; colour and medium metadata deliberately remain in physical slots.
// Use output_sources to recover semantic colour/ancestry for a Pythia8 export.
// Closed gluon loops can be left unconsumed by the stock routine: callers must
// inspect unconsumed_sources before exporting, rather than silently losing them.
StringBuildResult make_vacuum_strings(std::vector<CompressionRecord>&,std::size_t active,bool compress);
// Literal HADROTYPE=1 boundary. Generated quark halves map to their original
// gluon slot; artificial endpoints use size_t::max as their output source.
StringBuildResult make_minimum_strings(std::vector<CompressionRecord>&,std::size_t active,
 bool compress,double max_rapidity,const BranchingSampler&,Random&);
}
