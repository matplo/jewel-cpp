// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/recoil.hpp"
#include <cmath>
#include <stdexcept>
void check(bool b){if(!b)throw std::runtime_error("recoil contract failed");}
template<class F>void rejects(F f){bool bad=false;try{f();}catch(const std::domain_error&){bad=true;}check(bad);}
int main(){
 jewel::RecoilStorage storage;storage.capacity=3;
 jewel::Momentum incoming{0,0,0,1},outgoing{2,0,0,std::sqrt(5.)};
 jewel::store_recoil(storage,{0,true,false},21,incoming,1,outgoing,false,false);
 check(storage.used==1&&storage.records[0].flavour==21&&storage.records[0].centre.energy==1);
 check(storage.records[0].dummy.energy==1e-6&&storage.records[0].dummy_mass==1e-6);
 jewel::store_recoil(storage,{2,false,true},2,incoming,1,outgoing,false,false);
 auto r=storage.records[1];check(r.centre.px==2&&r.centre_mass<0&&r.dummy_mass<0);
 // Reuse a populated slot: modes 0/1 leave it untouched for soft recoils.
 storage.used=1;
 jewel::store_recoil(storage,{1,true,false},21,incoming,1,outgoing,true,false);
 check(storage.used==2&&storage.records[1].flavour==r.flavour&&storage.records[1].dummy.px==r.dummy.px);
 auto snapshot=storage;
 jewel::store_recoil(storage,{0,false,false},21,incoming,1,outgoing,false,false);check(storage.used==2);
 jewel::store_recoil(storage,{0,true,false},21,incoming,1,outgoing,false,true);check(storage.used==2);
 jewel::store_recoil(storage,{3,true,false},2,outgoing,1,incoming,true,false);
 check(storage.used==3&&storage.records[2].centre.energy<0&&storage.records[2].dummy.px<0);
 jewel::store_recoil(storage,{0,true,false},21,incoming,1,outgoing,false,false);check(storage.used==3);
 storage=snapshot;check(storage.used==2&&storage.records.size()==2);
 rejects([&]{jewel::store_recoil(storage,{4,true,false},21,incoming,1,outgoing,false,false);});
 storage.used=4;rejects([&]{jewel::store_recoil(storage,{0,true,false},21,incoming,1,outgoing,false,false);});
 storage={};rejects([&]{jewel::store_recoil(storage,{2,true,false},21,incoming,1,incoming,false,false);});
}
