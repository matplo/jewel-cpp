#include "jewel/subtraction.hpp"
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
struct Rand: jewel::Random {double uniform()override{return double(std::rand())/double(RAND_MAX);}};
int main(int argc,char** argv){
 auto policy=jewel::SubtractionMassPolicy::Reference;
 if(argc==2&&std::string(argv[1])=="--signed-mass")policy=jewel::SubtractionMassPolicy::SignedMass;else if(argc!=1)return 2;
 std::size_t n,m;unsigned seed;double radius;
 std::cout<<std::setprecision(17);
 while(std::cin>>n>>m>>radius>>seed){
  std::vector<jewel::SubtractionParticle> particles(n);std::vector<jewel::Momentum> thermal(m);
  for(auto& p:particles)std::cin>>p.pdg>>p.momentum.px>>p.momentum.py>>p.momentum.pz>>p.momentum.energy>>p.mass;
  for(auto& p:thermal)std::cin>>p.px>>p.py>>p.pz>>p.energy;
  if(!std::cin)return 2;
  std::srand(seed);Rand random;auto result=jewel::subtract_thermal_momenta(particles,thermal,radius,random,policy);
  std::cout<<result.particles.size()<<'\n';
  for(auto p:result.particles)std::cout<<p.pdg<<' '<<p.momentum.px<<' '<<p.momentum.py<<' '<<p.momentum.pz<<' '<<p.momentum.energy<<' '<<p.mass<<'\n';
 }
}
