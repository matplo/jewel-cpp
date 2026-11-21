#include "jewel/legacy_mpi_channels.hpp"
#include <iostream>
#include <iomanip>
#include <fstream>
int main(int argc,char** argv){try{
 std::vector<jewel::LegacyGluonDecay> decays;double fudge=1;
 if(argc>2)return 2;
 if(argc==2){std::ifstream config(argv[1]);int n;
  if(!(config>>fudge>>n)||n<0||n>400)return 2;decays.resize(n);
  for(auto& d:decays)if(!(config>>d.mass_a>>d.mass_b>>d.secondary_weight>>d.mode))return 2;
  std::string extra;if(config>>extra)return 2;
 }
std::cout<<std::setprecision(17);jewel::LegacyMpiChannelInput p;
 while(std::cin>>p.s>>p.t>>p.u>>p.cos_theta>>p.common_factor>>p.alpha_s>>p.asymmetric_factor>>p.annihilation_weight>>p.interference){
  for(auto& x:p.masses)if(!(std::cin>>x))return 2;
  for(auto& x:p.enabled){int b;if(!(std::cin>>b)||(b!=0&&b!=1))return 2;x=b;}
  std::array<double,11>a,b;for(auto& x:a)if(!(std::cin>>x))return 2;for(auto& x:b)if(!(std::cin>>x))return 2;
  if(argc==2)p.annihilation_weight=jewel::legacy_gluon_width(p.s,decays,fudge).annihilation_weight;
  auto r=jewel::legacy_mpi_channels(p,a,b);std::cout<<r.channels.size()<<' '<<r.total<<'\n';
  for(auto c:r.channels)std::cout<<c.incoming_a<<' '<<c.incoming_b<<' '<<c.code<<' '<<c.matrix_weight<<' '<<c.pdf_weighted<<'\n';
 }if(!std::cin.eof())return 2;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}}
