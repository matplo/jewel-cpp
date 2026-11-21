// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/medium.hpp"
#include <iostream>
#include <iomanip>
#include <memory>
#include <stdexcept>
int main() {
 try {
 jewel::ExponentialIntegral ei;
 std::unique_ptr<jewel::ExpandingMedium> medium;
 std::string op; std::cout<<std::scientific<<std::setprecision(17);
 while(std::cin>>op) {
  if(op=="CONFIG") {
   jewel::MediumConfig c; int woods;
   std::cin>>c.mass_number>>c.nf>>woods>>c.tau_initial>>c.temperature_initial>>c.temperature_cutoff
    >>c.nuclear_density>>c.skin_depth>>c.sigma_nn>>c.impact_parameter>>c.max_rapidity>>c.debye_floor>>c.debye_scale;
   if(!std::cin) throw std::runtime_error("bad configuration");
   c.woods_saxon=woods!=0; medium=std::make_unique<jewel::ExpandingMedium>(c);
  } else if(op=="EI") { double x; if(!(std::cin>>x)) throw std::runtime_error("bad EI input"); std::cout<<ei(x)<<'\n'; }
  else if(op=="LOW"||op=="HIGH"||op=="SCAT") {
   jewel::Kernels k; double x,y;
   if(!(std::cin>>k.nf>>x>>y>>k.lambda_shower>>k.lambda_scattering)) throw std::runtime_error("bad kernel input");
   double v=op=="LOW"?ei.intpqglow(x,y,k):op=="HIGH"?ei.intpqghigh(x,y,k):ei.scatprimfunc(x,y,k);
   std::cout<<v<<'\n';
  }
  else if(op=="EITABLE") { for(auto row:ei.table()) std::cout<<row[0]<<' '<<row[1]<<' '<<row[2]<<'\n'; }
  else {
   if(!medium) throw std::runtime_error("CONFIG required");
   if(op=="CROSS") { for(auto row:medium->cross_section_table()) std::cout<<row[0]<<' '<<row[1]<<' '<<row[2]<<'\n'; }
   else if(op=="TABLE") { for(auto row:medium->thickness_table()) std::cout<<row[0]<<' '<<row[1]<<'\n'; }
   else if(op=="THICK") { double x,y; if(!(std::cin>>x>>y)) throw std::runtime_error("bad point"); std::cout<<medium->thickness(x,y)<<'\n'; }
   else if(op=="PART") { double x,y,z,t; if(!(std::cin>>x>>y>>z>>t)) throw std::runtime_error("bad pair"); std::cout<<medium->participants(x,y,z,t)<<'\n'; }
   else if(op=="POINT") {
    jewel::SpaceTime v; if(!(std::cin>>v.x>>v.y>>v.z>>v.t)) throw std::runtime_error("bad position");
    auto s=medium->sample(v); std::cout<<s.temperature<<' '<<s.debye_mass<<' '<<s.scatterer_mass<<' '<<s.density<<'\n';
   } else throw std::runtime_error("unknown operation");
  }
 }
 } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 2; }
}
