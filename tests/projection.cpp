#include "jewel/projection.hpp"
#include <cmath>
#include <stdexcept>
void check(bool ok){if(!ok)throw std::runtime_error("projection contract failed");}
template<class F>void rejects(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed);}
int main(){
 std::vector<jewel::ProjectionParticle> input{{{3,4,0,4},-3,2},{{-3,-4,0,6},1,2}};
 auto off=jewel::project_on_shell(input,0);check(off.particles[0].momentum.energy==4&&off.particles[0].mass==-3);
 auto local=jewel::project_on_shell(input,1);check(local.particles[0].momentum.px==3&&local.particles[0].momentum.energy==std::sqrt(29.)&&local.particles[0].mass==2);
 check(input[0].momentum.energy==4);
 input={{{1,0,0,1.1},.1,.14},{{-1,0,0,1.1},.1,.14}};
 auto collective=jewel::project_on_shell(input,2);check(collective.massless_converged&&collective.massive_converged&&!collective.insufficient_energy);
 check(std::abs(collective.particles[0].momentum.energy+collective.particles[1].momentum.energy-2.2)<2.2e-6);
 input[0].nominal_mass=input[1].nominal_mass=1.1*(1-1e-12);
 auto threshold=jewel::project_on_shell(input,2);
 check(!threshold.massive_converged&&threshold.massive_iterations==10);
 input[0].nominal_mass=input[1].nominal_mass=5;
 auto insufficient=jewel::project_on_shell(input,2);check(insufficient.insufficient_energy&&insufficient.massive_iterations==0&&insufficient.particles[0].mass==5);
 rejects([&]{jewel::project_on_shell(input,3);});rejects([&]{jewel::project_on_shell(input,2,3);});
 input[0].nominal_mass=-1;rejects([&]{jewel::project_on_shell(input,1);});
 rejects([&]{jewel::project_on_shell({{{1e308,0,0,1},0,1}},1);});
 check(jewel::project_on_shell({},2).particles.empty());
 rejects([&]{jewel::project_on_shell({{{1,0,0,1},0,0}},2);});
}
