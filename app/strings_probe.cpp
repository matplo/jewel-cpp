// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/strings.hpp"
#include <iomanip>
#include <iostream>
int main(){try{
 std::size_t count;int compress;std::cout<<std::setprecision(17);
 while(std::cin>>count>>compress){
  if(count>22990)throw std::runtime_error("invalid count");
  std::vector<jewel::CompressionRecord> rows(count);
  for(auto& r:rows){for(auto& v:r.k)std::cin>>v;for(auto& v:r.p)std::cin>>v;for(auto& v:r.v)std::cin>>v;for(auto& v:r.production)std::cin>>v;std::cin>>r.colour>>r.anticolour;}
  if(!std::cin)throw std::runtime_error("invalid string input");
  auto result=jewel::make_vacuum_strings(rows,count,compress!=0);
  std::cout<<result.active<<' '<<result.discarded<<'\n';
  for(std::size_t i=0;i<result.active;++i){const auto& r=rows[i];for(auto v:r.k)std::cout<<v<<' ';for(auto v:r.p)std::cout<<v<<' ';for(auto v:r.v)std::cout<<v<<' ';for(auto v:r.production)std::cout<<v<<' ';std::cout<<r.colour<<' '<<r.anticolour<<'\n';}
  std::cerr<<"UNCONSUMED "<<result.unconsumed_sources.size()<<'\n';
 }
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
