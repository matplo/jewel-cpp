#include "jewel/pythia8_isr.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
void check(bool b){if(!b)throw std::runtime_error("ISR recovery contract failed");}
int main(int argc,char** argv){
 if(argc!=2)throw std::invalid_argument("supply installed PDF SET/MEMBER");
 jewel::PythiaInputConfig c;c.seed=2600421;c.initial_state_radiation=true;c.multiple_parton_interactions=true;c.external_pdf=argv[1];
 c.hard_alpha_s=c.isr_alpha_s=0.11754434294459773;c.hard_alpha_s_order=c.isr_alpha_s_order=1;c.alpha_s_nfmax=5;c.isr_alpha_s_cmw=false;
 jewel::Pythia8Input input(c);int recovered=0,corrupt_rejected=0;
 for(int attempt=1;attempt<=1500;++attempt){auto e=input.next();check(bool(e));
  auto systems=jewel::reconstruct_subsystem_isr_frames(*e);
  for(const auto& frames:systems)for(const auto& f:frames)if(f.source_rounding_recovered){
   ++recovered;check(attempt==385||attempt==1089||attempt==1355);
   check(std::sqrt(f.recovered_scale_squared)==e->particles[f.emitted-1].scale);
   // Corrupt one checked copy while leaving source fractions/scale intact.
   // Recovery must not conceal a genuinely inconsistent record.
   auto corrupt=*e;corrupt.particles[f.copies.front()[1]-1].momentum.px+=.01;
   bool failed=false;try{jewel::reconstruct_subsystem_isr_frames(corrupt);}catch(const std::domain_error&){failed=true;}
   check(failed);++corrupt_rejected;
  }
 }
 check(recovered==3&&corrupt_rejected==3);
 std::cout<<"1500 events mapped; 3 bounded recoveries; 3 corrupted copies rejected\n";
}
