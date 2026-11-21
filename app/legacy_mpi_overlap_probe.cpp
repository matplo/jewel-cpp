#include "jewel/legacy_mpi.hpp"
#include <iostream>
#include <iomanip>
int main(){try{
 std::cout<<std::setprecision(17);
 double f,r,sigma,nd,u,v;
 while(std::cin>>f>>r>>sigma>>nd>>u>>v){
  jewel::LegacyMpiOverlap model(f,r,sigma,nd);auto s=model.state();
  jewel::ReplayRandom random({u,v});auto p=model.sample_hard(random);
  std::cout<<s.integral<<' '<<s.overlap_average<<' '<<s.overlap_normalization<<' '
   <<s.mean_impact<<' '<<s.dividing_impact<<' '<<s.k_factor<<' '
   <<p.squared_impact<<' '<<p.enhancement<<' '<<p.relative_impact<<' '<<random.checkpoint()<<'\n';
 }
 if(!std::cin.eof())return 2;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}}
