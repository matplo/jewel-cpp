#include "jewel/legacy_mpi.hpp"
#include <iostream>
#include <iomanip>
int main(){try{
 std::cout<<std::setprecision(17);double upper,s,a,e,b,nd,u;
 while(std::cin>>upper>>s>>a>>e>>b>>nd>>u){
  jewel::LegacyMpiScaleSampler sampler(s,a,e,b,nd);
  jewel::ReplayRandom random({u});auto r=sampler.next(upper,random);
  std::cout<<sampler.factor()<<' '<<r.xt_squared<<' '<<r.proposed<<' '<<random.checkpoint()<<'\n';
 }
 if(!std::cin.eof())return 2;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}}
