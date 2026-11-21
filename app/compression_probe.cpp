// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/compression.hpp"
#include <iostream>
#include <iomanip>
int main(){
 std::cout<<std::setprecision(17);
 std::size_t n,selected;
 while(std::cin>>n>>selected){
  std::vector<jewel::CompressionRecord> records(n);
  for(auto& r:records){
   for(auto& x:r.k)std::cin>>x;
   for(auto* values:{&r.p,&r.v,&r.production})for(auto& x:*values)std::cin>>x;
   std::cin>>r.colour>>r.anticolour>>r.fraction>>r.angle>>r.quark_pair>>r.angular_required>>r.evolution_time>>r.medium_induced;
  }
  if(!std::cin)return 2;
  auto result=jewel::compress_records(records,selected);
  std::cout<<result.active<<' '<<result.selected<<' '<<result.room_left<<'\n';
  for(const auto& r:records){
   for(auto x:r.k)std::cout<<x<<' ';
   for(auto* values:{&r.p,&r.v,&r.production})for(auto x:*values)std::cout<<x<<' ';
   std::cout<<r.colour<<' '<<r.anticolour<<' '<<r.fraction<<' '<<r.angle<<' '<<r.quark_pair<<' '<<r.angular_required<<' '<<r.evolution_time<<' '<<r.medium_induced<<'\n';
  }
 }
}
