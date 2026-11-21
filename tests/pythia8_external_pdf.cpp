// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/pythia8_input.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
void check(bool x){if(!x)throw std::runtime_error("external PDF contract failed");}
int main(int argc,char** argv){
 if(argc!=2)throw std::invalid_argument("supply installed external SET/MEMBER");
 jewel::PythiaInputConfig config;config.external_pdf=argv[1];config.hard_alpha_s=.118;config.hard_alpha_s_order=2;
 jewel::Pythia8Input a(config),b(config);int accepted=0;
 for(int i=0;i<100;++i){auto x=a.next(),y=b.next();check(x.has_value()==y.has_value());if(!x)continue;++accepted;
  check(x->process_code==y->process_code&&x->weight==y->weight&&x->particles.size()==y->particles.size());
  for(std::size_t j=0;j<x->particles.size();++j){const auto& p=x->particles[j];const auto& q=y->particles[j];
   check(p.pdg==q.pdg&&p.colour==q.colour&&p.anticolour==q.anticolour);
   check(p.momentum.px==q.momentum.px&&p.momentum.py==q.momentum.py&&p.momentum.pz==q.momentum.pz&&p.momentum.energy==q.momentum.energy);
  }
 }
 auto stats=a.statistics();check(accepted>0&&stats.returned==static_cast<unsigned>(accepted)&&stats.sigma_mb>0&&std::isfinite(stats.sigma_error_mb));
 std::cout<<accepted<<" reproducible external-PDF hard events\n";
}
