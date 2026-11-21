#include "jewel/pythia8_isr.hpp"
#include <iostream>
#include <iomanip>
int main(){jewel::IsrRecoilFrame f;double e,x,y,z;while(std::cin>>f.matrix[0]){for(int i=1;i<16;++i)std::cin>>f.matrix[i];std::cin>>e>>x>>y>>z;auto p=jewel::apply_isr_frame(f,{x,y,z,e});std::cout<<std::setprecision(17)<<p.energy<<' '<<p.px<<' '<<p.py<<' '<<p.pz<<'\n';}}
