// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/recoil.hpp"
#include <iostream>
#include <iomanip>
int main(){try{
 std::cout<<std::scientific<<std::setprecision(17);
 int mode,write,subtract,soft,rejected,flavour;std::size_t used,capacity;
 while(std::cin>>mode>>write>>subtract>>soft>>rejected>>used>>capacity>>flavour){
  jewel::Momentum p,q;double mass;
  if(!(std::cin>>p.px>>p.py>>p.pz>>p.energy>>mass>>q.px>>q.py>>q.pz>>q.energy))throw std::runtime_error("bad recoil input");
  jewel::RecoilStorage storage;storage.used=used;storage.capacity=capacity;storage.records.resize(capacity);
  for(auto& record:storage.records)record={-99,{1,2,3,4},5,{6,7,8,9},10};
  jewel::store_recoil(storage,{mode,bool(write),bool(subtract)},flavour,p,mass,q,soft,rejected);
  std::cout<<storage.used;
  for(const auto& r:storage.records)std::cout<<' '<<r.flavour<<' '<<r.centre.px<<' '<<r.centre.py<<' '<<r.centre.pz<<' '<<r.centre.energy<<' '<<r.centre_mass<<' '<<r.dummy.px<<' '<<r.dummy.py<<' '<<r.dummy.pz<<' '<<r.dummy.energy<<' '<<r.dummy_mass;
  std::cout<<'\n';
 }
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}}
