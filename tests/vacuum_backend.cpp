// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/scattering.hpp"
#include <stdexcept>
void check(bool value){if(!value)throw std::runtime_error("vacuum backend contract failed");}
int main(){
 jewel::MediumModel vacuum;jewel::ReplayRandom empty({});
 auto vertex=vacuum.sample_vertex(empty);check(vertex.x==0&&vertex.y==0&&vertex.z==0&&vertex.t==0&&empty.checkpoint()==0);
 auto maximum=vacuum.maximum_scatterer(true);check(maximum.mass==.5&&maximum.momentum.energy==.5&&maximum.momentum.pz==0);
 check(vacuum.bounds().lifetime_max==0&&vacuum.sample({1,2,3,4}).density==0);
 jewel::ScatteringRates rates({},1,100,1,vacuum,true);
 auto hit=rates.next_collision({30,0,0,30},0,21,{0,0,0,0},0,1,0,1,false,empty);
 check(!hit.collision&&hit.elapsed==0&&empty.checkpoint()==0);
 check(rates.cross_section({30,0,0,30},0,0,'G','C',{},1)==0);
 bool rejected=false;
 try{rates.integral(2,0,jewel::RateChannel::GA);}catch(const std::domain_error&){rejected=true;}
 check(rejected);rejected=false;
 try{vacuum.sample_scatterer({},empty);}catch(const std::domain_error&){rejected=true;}
 check(rejected&&empty.checkpoint()==0);
}
