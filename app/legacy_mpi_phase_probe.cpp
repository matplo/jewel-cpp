#include "jewel/legacy_mpi.hpp"
#include <iostream>
#include <iomanip>
int main(){try{
 std::cout<<std::setprecision(17);double xt,a,b;std::array<double,3>c;
 while(std::cin>>xt>>c[0]>>c[1]>>c[2]>>a>>b){
  std::vector<double> tape(5);for(double& u:tape)if(!(std::cin>>u))return 2;
  jewel::ReplayRandom random(tape);auto p=jewel::sample_legacy_mpi_phase_space(xt,c,random);
  std::cout<<p.tau<<' '<<p.rapidity<<' '<<p.cos_theta<<' '<<p.x1<<' '<<p.x2<<' '
   <<p.momentum_available(a,b)<<' '<<random.checkpoint()<<'\n';
 }
 if(!std::cin.eof())return 2;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}}
