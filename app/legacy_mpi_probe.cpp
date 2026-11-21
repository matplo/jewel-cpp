#include "jewel/legacy_mpi.hpp"
#include <iostream>
#include <iomanip>
int main(){try{
 std::cout<<std::setprecision(17);
 double xt,screening,enhancement,sigma,norm,rescale;int bypass;
 while(std::cin>>xt>>screening>>enhancement>>sigma>>norm>>rescale>>bypass){
  std::array<double,20>sums;std::array<int,20>counts;
  for(auto& x:sums)if(!(std::cin>>x))return 2;
  for(auto& x:counts)if(!(std::cin>>x))return 2;
  auto r=jewel::LegacyMpiTable(sums,counts,norm).hard_weight(xt,screening,enhancement,sigma,bypass!=0,rescale);
  std::cout<<r.bin_coordinate<<' '<<r.bin<<' '<<r.cross_section_above<<' '<<r.probability<<'\n';
 }
 if(!std::cin.eof())return 2;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}}
