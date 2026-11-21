// SPDX-License-Identifier: GPL-2.0-or-later
// Harness-only source of matched uniform draws, replacing the finite tape.
// This is not the Pythia6 RNG, and does not implement any reference physics.
#include <cstdint>
extern "C" double reference_uniform(std::int64_t seed,std::int64_t draw){
 std::uint64_t bits=std::uint64_t(seed)+0x9e3779b97f4a7c15ULL*std::uint64_t(draw);
 bits=(bits^(bits>>30))*0xbf58476d1ce4e5b9ULL;
 bits=(bits^(bits>>27))*0x94d049bb133111ebULL;
 bits=bits^(bits>>31);
 return (double(bits>>12)+0.5)/4503599627370496.;
}
