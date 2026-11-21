#include "jewel/subtraction.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>
void check(bool x){if(!x)throw std::runtime_error("subtraction contract failed");}
template<class F>void rejects(F f){bool threw=false;try{f();}catch(const std::exception&){threw=true;}check(threw);}
int main(){
 jewel::ReplayRandom random({.1,.5,.9});
 std::vector<jewel::SubtractionParticle> particles{{17,211,{10,0,0,10},0,false}};
 auto result=jewel::subtract_thermal_momenta(particles,{{3,0,0,3}},0,random);
 check(result.particles.size()==1&&result.particles[0].source==17&&!result.particles[0].thermal_residual);
 check(result.particles[0].momentum.px==7&&particles[0].momentum.px==10&&random.checkpoint()==0);
 result=jewel::subtract_thermal_momenta(particles,{{13,0,0,13}},0,random);
 check(result.particles.size()==1&&result.particles[0].source==0&&result.particles[0].thermal_residual);
 check(result.particles[0].momentum.px==3&&result.particles[0].pdg==211&&random.checkpoint()==1);
 std::vector<jewel::SubtractionParticle> nearly_null{{1,22,{1,0,0,std::nextafter(1.,0.)},0,false}};
 rejects([&]{jewel::subtract_thermal_momenta(nearly_null,{},.4,random);});
 auto repaired=jewel::subtract_thermal_momenta(nearly_null,{},.4,random,jewel::SubtractionMassPolicy::SignedMass);
 check(repaired.spacelike_mass_fields==1&&repaired.particles[0].mass<0);
 check(repaired.particles[0].momentum.energy==nearly_null[0].momentum.energy);
 nearly_null[0].momentum.energy=.5;
 auto spacelike=jewel::subtract_thermal_momenta(nearly_null,{},.4,random,jewel::SubtractionMassPolicy::SignedMass);
 check(spacelike.particles[0].mass<0&&spacelike.particles[0].momentum.energy==.5);
 check(jewel::subtract_thermal_momenta({}, {}, .4,random).particles.empty());
 result=jewel::subtract_thermal_momenta({{1,22,{0,0,1,1},0,false}},{{0,0,1,1}},.4,random);
 check(result.skipped_particles==1&&result.skipped_thermals==1&&result.particles.empty());
 rejects([&]{jewel::subtract_thermal_momenta({}, {}, -.1,random);});
 rejects([&]{jewel::subtract_thermal_momenta({}, {}, std::numeric_limits<double>::quiet_NaN(),random);});
 rejects([&]{jewel::subtract_thermal_momenta({},{{0,0,0,std::numeric_limits<double>::infinity()}},.4,random);});
}
