// SPDX-License-Identifier: GPL-2.0-or-later
// Translated from JEWEL 2.6.0 PUTPARTICLESONSHELL; see NOTICE.
#include "jewel/projection.hpp"
#include "jewel/kinematics.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace jewel {
namespace {
double p2(Momentum p){return p.px*p.px+p.py*p.py+p.pz*p.pz;}
void require(bool ok,const char* message){if(!ok)throw std::domain_error(message);}
}
ProjectionResult project_on_shell(const std::vector<ProjectionParticle>& input,int mode,std::size_t first){
 require(mode>=0&&mode<=2,"invalid on-shell mode");
 ProjectionResult result;result.particles=input;
 if(mode==0)return result;
 for(const auto& p:input){for(double x:{p.momentum.px,p.momentum.py,p.momentum.pz,p.momentum.energy,p.nominal_mass})require(std::isfinite(x),"nonfinite projection input");require(p.nominal_mass>=0,"negative nominal mass");}
 if(mode==1){for(auto& p:result.particles){p.momentum.energy=std::sqrt(p2(p.momentum)+p.nominal_mass*p.nominal_mass);p.mass=p.nominal_mass;require(std::isfinite(p.momentum.energy),"nonfinite projection output");}return result;}
 require(first<=input.size(),"invalid projection range");if(first==input.size())return result;
 auto& particles=result.particles;Momentum total{};
 for(std::size_t i=first;i<particles.size();++i){auto p=particles[i].momentum;total.px+=p.px;total.py+=p.py;total.pz+=p.pz;total.energy+=p.energy;}
 require(total.energy>0&&p2(total)<total.energy*total.energy,"projection requires timelike total momentum");
 std::array<double,3> beta{total.px/total.energy,total.py/total.energy,total.pz/total.energy};
 for(std::size_t i=first;i<particles.size();++i)rotate_boost(particles[i].momentum,nullptr,0,0,{-beta[0],-beta[1],-beta[2]});
 double energy=0,masses=0;
 for(std::size_t i=first;i<particles.size();++i){auto p=particles[i].momentum;energy+=p.energy;masses+=std::sqrt(std::max(p.energy*p.energy-p2(p),0.));}
 require(energy>0&&masses<energy,"undefined massless projection scale");
 std::vector<double> new_energy(particles.size());
 auto rescale=[&](bool massive){
  double x=1/std::sqrt(1-(masses/energy)*(masses/energy)),accuracy=1e-6*energy;
  bool converged=false;int iteration=0;
  for(int j=1;j<=10;++j){
   iteration=j;double f=-energy,g=0,x2=x*x;
   for(std::size_t i=first;i<particles.size();++i){const auto& p=particles[i];double q=massive?p.momentum.energy*p.momentum.energy:p2(p.momentum);
    new_energy[i]=std::sqrt((massive?p.nominal_mass*p.nominal_mass:0)+x2*q);
    require(new_energy[i]>0&&std::isfinite(new_energy[i]),"undefined projection derivative");
    f+=new_energy[i];g+=q/new_energy[i];
   }
   if(std::abs(f)<accuracy){converged=true;break;}
   require(x*g!=0&&std::isfinite(g),"undefined projection iteration");x-=f/(x*g);
  }
  // Preserve the source's ten-step limit and last computed energies, including
  // its nonconverged exit. Convergence is reported explicitly to the caller.
  for(std::size_t i=first;i<particles.size();++i){auto& p=particles[i];p.momentum.px*=x;p.momentum.py*=x;p.momentum.pz*=x;p.momentum.energy=new_energy[i];p.mass=std::sqrt(std::abs(p.momentum.energy*p.momentum.energy-p2(p.momentum)));}
  if(massive){result.massive_iterations=iteration;result.massive_converged=converged;}
  else{result.massless_iterations=iteration;result.massless_converged=converged;}
 };
 rescale(false);energy=0;masses=0;
 for(std::size_t i=first;i<particles.size();++i){energy+=particles[i].momentum.energy;masses+=particles[i].nominal_mass;}
 if(energy>masses)rescale(true);else result.insufficient_energy=true;
 for(std::size_t i=first;i<particles.size();++i){auto& p=particles[i];rotate_boost(p.momentum,nullptr,0,0,beta);p.momentum.energy=std::sqrt(p2(p.momentum)+p.nominal_mass*p.nominal_mass);p.mass=p.nominal_mass;}
 for(const auto& p:particles)for(double v:{p.momentum.px,p.momentum.py,p.momentum.pz,p.momentum.energy,p.mass})require(std::isfinite(v),"nonfinite projection output");
 return result;
}
}
