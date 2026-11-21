#include "jewel/projection.hpp"
#include <iostream>
#include <iomanip>
int main(){int n,mode,first;std::cout<<std::setprecision(17);
 while(std::cin>>n>>mode>>first){if(n<0)return 2;std::vector<jewel::ProjectionParticle> p(n);
  for(auto& x:p)std::cin>>x.momentum.px>>x.momentum.py>>x.momentum.pz>>x.momentum.energy>>x.mass>>x.nominal_mass;
  if(!std::cin)return 2;auto r=jewel::project_on_shell(p,mode,first);std::cout<<r.particles.size()<<'\n';
  for(auto x:r.particles)std::cout<<x.momentum.px<<' '<<x.momentum.py<<' '<<x.momentum.pz<<' '<<x.momentum.energy<<' '<<x.mass<<'\n';
 }
}
