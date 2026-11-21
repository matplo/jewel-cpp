#include "jewel/legacy_mpi.hpp"
#include <iostream>
#include <iomanip>
int main(){try{
 std::cout<<std::setprecision(17);double sigma,bound,ecm,xt,x1,x2,c,attach,gluon,strange,pi;
 while(std::cin>>sigma>>bound>>ecm>>xt>>x1>>x2>>c>>attach>>gluon>>strange>>pi){
  std::vector<double> tape(4);for(auto& u:tape)if(!(std::cin>>u))return 2;
  jewel::ReplayRandom random(tape);
  auto pair=jewel::sample_legacy_mpi_pair(sigma,bound,ecm,xt,x1,x2,c,attach,gluon,strange,pi,random);
  auto p=pair.value_or(jewel::LegacyMpiPair{});
  std::cout<<bool(pair)<<' '<<random.checkpoint()<<' '<<p.flavour_draw<<' '<<p.pt<<' '<<p.phi<<' '<<p.pdg_a<<' '<<p.pdg_b;
  for(auto v:{p.a,p.b})std::cout<<' '<<v.px<<' '<<v.py<<' '<<v.pz<<' '<<v.energy;
  std::cout<<'\n';
 }if(!std::cin.eof())return 2;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}}
