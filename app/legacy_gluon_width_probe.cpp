#include "jewel/legacy_mpi_channels.hpp"
#include <iostream>
#include <iomanip>
int main(){try{std::cout<<std::setprecision(17);double s,f;int n;
 while(std::cin>>s>>f>>n){if(n<0||n>400)return 2;std::vector<jewel::LegacyGluonDecay>d(n);
  for(auto& c:d)if(!(std::cin>>c.mass_a>>c.mass_b>>c.secondary_weight>>c.mode))return 2;
  auto w=jewel::legacy_gluon_width(s,d,f);std::cout<<w.partial_total;
  for(double x:w.enabled)std::cout<<' '<<x;std::cout<<' '<<w.annihilation_weight;
  for(double x:w.partial)std::cout<<' '<<x;std::cout<<'\n';
 }if(!std::cin.eof())return 2;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}}
