// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/pythia8_input.hpp"
#include <cmath>
#include <stdexcept>
void check(bool value){if(!value)throw std::runtime_error("Pythia8 input contract failed");}
int main(){
 jewel::PythiaInputConfig config;config.seed=2600317;
 jewel::Pythia8Input first(config),second(config);unsigned accepted=0;
 for(int i=0;i<20;++i){auto a=first.next(),b=second.next();check(a.has_value()==b.has_value());if(!a)continue;++accepted;
  check(a->process_code==b->process_code&&a->weight==b->weight&&a->particles.size()==b->particles.size());
  check(a->hard_incoming==b->hard_incoming&&a->hard_outgoing==b->hard_outgoing&&a->hard_final==b->hard_final);
  for(std::size_t j=0;j<a->particles.size();++j){const auto& x=a->particles[j];const auto& y=b->particles[j];
   check(x.status==y.status&&x.pdg==y.pdg&&x.mothers==y.mothers&&x.daughters==y.daughters&&x.colour==y.colour&&x.anticolour==y.anticolour);
   check(x.momentum.px==y.momentum.px&&x.momentum.py==y.momentum.py&&x.momentum.pz==y.momentum.pz&&x.momentum.energy==y.momentum.energy);
  }
  auto wrong_copy=*a;wrong_copy.hard_final[0]=wrong_copy.hard_outgoing[0];bool copy_rejected=false;
  try{jewel::validate_hard_snapshot(wrong_copy);}catch(const std::domain_error&){copy_rejected=true;}check(copy_rejected);
  auto broken=*a;broken.hard_outgoing[1]=broken.hard_outgoing[0];bool rejected=false;
  try{jewel::validate_hard_snapshot(broken);}catch(const std::domain_error&){rejected=true;}check(rejected);
 }
 auto s=first.statistics();check(accepted>0&&s.attempts==20&&s.returned==accepted&&s.failed==20-accepted);
 check(s.sigma_mb>0&&std::isfinite(s.sigma_error_mb));
}
