// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/hard_initialization.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>
void check(bool condition){if(!condition)throw std::runtime_error("massless preparation contract failed");}
bool same(jewel::Momentum a,jewel::Momentum b){return a.px==b.px&&a.py==b.py&&a.pz==b.pz&&a.energy==b.energy;}
int main(){
 jewel::ShowerParticle first,second,emitted;
 for(auto* p:{&first,&second,&emitted}){p->particle.status=1;p->particle.production={1,2,3,4};p->parent=77;p->decay_time=9;}
 first.particle.momentum={3,4,0,5};second.particle.momentum={-3,-4,0,5};
 emitted.particle.momentum={1,2,3,std::sqrt(18.)};emitted.record_mass=2;
 jewel::make_parton_massless(emitted,first,second);
 check(emitted.record_mass==0);
 check(std::abs(emitted.particle.momentum.energy-std::sqrt(14.))<1e-14);
 for(auto* p:{&first,&second,&emitted})check(p->parent==77&&p->decay_time==9&&p->particle.production.x==1&&p->particle.production.y==2&&p->particle.production.z==3&&p->particle.production.t==4);
 // Failure after transformations must not publish a partially changed record.
 first.particle.momentum={0,0,0,2};second.particle.momentum={0,0,0,2};
 const auto before_first=first.particle.momentum,before_second=second.particle.momentum,before_emitted=emitted.particle.momentum;
 bool rejected=false;
 try{jewel::make_parton_massless(emitted,first,second);}catch(const std::domain_error&){rejected=true;}
 check(rejected&&same(first.particle.momentum,before_first)&&same(second.particle.momentum,before_second)&&same(emitted.particle.momentum,before_emitted));
 rejected=false;
 try{jewel::make_parton_massless(first,first,second);}catch(const std::domain_error&){rejected=true;}
 check(rejected&&same(first.particle.momentum,before_first));
 // Combined preparation must roll back earlier conversions if a later one
 // fails. Both are near-on-shell light quarks inside acceptance.
 first.particle.momentum={3,4,12,13};first.particle.pdg=2;
 second.particle.momentum={-3,-4,-12,13};second.particle.pdg=21;
 emitted.particle.momentum={1,0,0,std::sqrt(1.+.33*.33)};
 emitted.record_mass=.33;emitted.particle.pdg=1;
 std::vector<jewel::ShowerParticle> records{first,second,emitted,emitted};
 records[3].generator_vertex.x=std::numeric_limits<double>::quiet_NaN();
 const auto original_hard=records[0].particle.momentum;
 rejected=false;
 try{jewel::prepare_surviving_partons(records,0,1,{.33,0,.33,.33},2);}
 catch(const std::domain_error&){rejected=true;}
 check(rejected&&same(records[0].particle.momentum,original_hard)&&records[2].record_mass==.33);
 // The above-nominal-mass branch precedes the eta cut; photons are inactive.
 records={first,second,emitted,emitted,emitted};
 records[2].particle.momentum={1,0,100,101};records[2].record_mass=.5;
 records[3].particle.momentum={1,0,100,101};
 records[4].particle.pdg=22;
 jewel::prepare_surviving_partons(records,0,1,{.33,0,.33,.33,0},2);
 check(records[2].particle.status==1&&records[2].record_mass==.5);
 check(records[3].particle.status==4&&records[3].record_mass==.33);
 check(records[4].particle.status==4);

}
