// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/core.hpp"
namespace jewel {
struct RecoilRecord {
 int flavour{};
 Momentum centre;
 double centre_mass{};
 Momentum dummy;
 double dummy_mass{};
};
struct RecoilStorage {
 // Allocated tail survives rollback, as in STORESCATCEN/STOREDUMMIES.
 std::vector<RecoilRecord> records;
 std::size_t used{},capacity{23000};
};
struct RecoilStorageConfig {
 int mode{}; // JEWEL RECMODE 0..3
 bool write_centres{},subtract{};
};
// Storage section of DOKINEMATICS; incoming/outgoing momenta are in the lab.
// A soft recoil in modes 0/1 reserves a slot without overwriting its contents.
void store_recoil(RecoilStorage&,RecoilStorageConfig,int flavour,
 Momentum incoming,double incoming_mass,Momentum outgoing,bool soft,bool rejected);
}
