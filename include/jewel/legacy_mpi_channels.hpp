// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <array>
#include <vector>
namespace jewel {
struct LegacyGluonDecay {
 double mass_a{},mass_b{},secondary_weight{1};
 int mode{1}; // MDME: negative skips; 0 partial only; 1..5 enabled bucket.
};
struct LegacyGluonWidth {
 std::vector<double> partial;
 double partial_total{};
 std::array<double,6> enabled{}; // Bucket 0 totals all enabled modes.
 double annihilation_weight{}; // Source sum of buckets 1..4, excludes 5.
};
LegacyGluonWidth legacy_gluon_width(double s,const std::vector<LegacyGluonDecay>&,
 double fudge=1);
struct LegacyMpiChannel {int incoming_a{},incoming_b{},code{};double matrix_weight{},pdf_weighted{};};
struct LegacyMpiChannelInput {
 double s{},t{},u{},cos_theta{},common_factor{},alpha_s{},asymmetric_factor{},annihilation_weight{};
 int interference{};
 std::array<double,5> masses{}; // d,u,s,c,b, GeV.
 std::array<bool,5> enabled{};
};
// PDF arrays are x*f(x,Q2), ordered -5..-1, gluon, 1..5.
struct LegacyMpiChannelResult {std::vector<LegacyMpiChannel> channels;double total{};};
// ISUB=96 factors and source-order PDF multiplication; PYWIDT sum, alpha_s
// and PDF evaluation/threshold suppression remain explicit upstream inputs.
LegacyMpiChannelResult legacy_mpi_channels(const LegacyMpiChannelInput&,
 const std::array<double,11>& pdf_a,const std::array<double,11>& pdf_b);
}
