// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/hard_initialization.hpp"
#include <cmath>
#include <stdexcept>
void check(bool value){if(!value)throw std::runtime_error("hard initialization contract failed");}
int main(){
 jewel::ShowerState state(-1e6);state.colour_max=101;state.append({});
 jewel::ShowerParticle q;q.particle.status=1;q.particle.pdg=1;q.particle.colour=101;q.particle.momentum={10,0,0,10};
 auto first=state.append(q,true,true);q.particle.pdg=-1;q.particle.colour=0;q.particle.anticolour=101;q.particle.momentum={-10,0,0,10};
 auto second=state.append(q,true,true);auto baseline=state.snapshot();
 jewel::BranchingSampler sampler({},1,1,0);jewel::HardInitializationConfig config;config.x=2;config.y=-3;
 jewel::ReplayRandom no_mass({.5});auto r=jewel::initialize_hard_dijet(state,first,second,config,sampler,no_mass);
 check(no_mass.checkpoint()==1&&r.trials==1);
 check(state.at(first).record_mass==0&&state.at(first).decay_time==20);
 check(state.at(first).particle.production.x==2&&state.at(second).particle.production.y==-3);
 check(state.at(first).evolution_time==-1e6&&state.at(first).fraction==0);
 config.pt_hat=20;state.restore(baseline);jewel::ReplayRandom empty({});bool rejected=false;
 try{jewel::initialize_hard_dijet(state,first,second,config,sampler,empty);}catch(const std::out_of_range&){rejected=true;}
 check(rejected&&state.at(first).particle.production.x==0&&state.at(first).particle.momentum.px==10);
 jewel::SplitMixRandom random(2600100);jewel::initialize_hard_dijet(state,first,second,config,sampler,random);state.validate();
 auto a=state.at(first).particle.momentum,b=state.at(second).particle.momentum;
 check(std::abs(a.energy+b.energy-20)<1e-10&&std::abs(a.px+b.px)<1e-10);
 // Disabling unused diagnostics preserves the entire sampled hard-leg result.
 auto ordinary=state;auto diagnostic_draws=random.checkpoint();
 state.restore(baseline);config.evaluate_colour_angles=false;
 jewel::SplitMixRandom no_angles(2600100);
 auto skipped=jewel::initialize_hard_dijet(state,first,second,config,sampler,no_angles);
 check(!skipped.colour_angles_evaluated&&no_angles.checkpoint()==diagnostic_draws);
 for(auto id:{first,second}){const auto& x=state.at(id);const auto& y=ordinary.at(id);
  check(x.record_mass==y.record_mass&&x.fraction==y.fraction&&x.angle==y.angle&&x.decay_time==y.decay_time&&x.quark_pair==y.quark_pair);
  check(x.particle.momentum.px==y.particle.momentum.px&&x.particle.momentum.py==y.particle.momentum.py&&x.particle.momentum.pz==y.particle.momentum.pz&&x.particle.momentum.energy==y.particle.momentum.energy);
 }
 state.restore(baseline);state.at(second).particle.anticolour=102;state.colour_max=102;
 jewel::SplitMixRandom junction_random(2600100);
 jewel::initialize_hard_dijet(state,first,second,config,sampler,junction_random);
 check(junction_random.checkpoint()==diagnostic_draws);
 config.first_angle_ordering=true;jewel::ReplayRandom forbidden({});rejected=false;
 try{jewel::initialize_hard_dijet(state,first,second,config,sampler,forbidden);}catch(const std::domain_error&){rejected=true;}
 check(rejected&&forbidden.checkpoint()==0);config.evaluate_colour_angles=true;
 // MPI switches override the primary hard scale and do not consult colour angles.
 state.restore(baseline);config.mpi_pair=true;config.mpi_fsr=false;config.mpi_sampling_channels=std::array<jewel::Channel,2>{jewel::Channel::QQ,jewel::Channel::QQ};config.first_angle_ordering=true;config.pt_hat=1000;
 for(auto id:{first,second}){state.at(id).particle.colour=0;state.at(id).particle.anticolour=0;}
 jewel::ReplayRandom no_mpi_fsr({.5});auto mpi=jewel::initialize_hard_dijet(state,first,second,config,sampler,no_mpi_fsr);
 check(no_mpi_fsr.checkpoint()==1&&mpi.colour_angles[0]==-1&&mpi.colour_angles[1]==-1);
 check(state.at(first).record_mass==0&&state.at(second).record_mass==0);
 config.mpi_fsr=true;state.restore(baseline);jewel::SplitMixRandom mpi_random(2600100);
 jewel::initialize_hard_dijet(state,first,second,config,sampler,mpi_random);
 auto mpi_a=state.at(first),mpi_b=state.at(second);auto draws=mpi_random.checkpoint();
 state.restore(baseline);config.pt_hat=0;jewel::SplitMixRandom mpi_repeat(2600100);
 jewel::initialize_hard_dijet(state,first,second,config,sampler,mpi_repeat);
 check(draws==mpi_repeat.checkpoint()&&state.at(first).record_mass==mpi_a.record_mass&&state.at(second).record_mass==mpi_b.record_mass);
 check(state.at(first).particle.momentum.energy==mpi_a.particle.momentum.energy&&state.at(second).particle.momentum.energy==mpi_b.particle.momentum.energy);
 config.mpi_sampling_channels.reset();jewel::ReplayRandom missing_channels({.5});bool missing_rejected=false;
 try{jewel::initialize_hard_dijet(state,first,second,config,sampler,missing_channels);}catch(const std::domain_error&){missing_rejected=true;}
 check(missing_rejected&&missing_channels.checkpoint()==0);
 // GETTHETAM suppresses angles for partners attached to beam slots 1/2.
 // Distinct incoming/original hard anchors beyond those slots retain the angle.
 for(int parent_slot=1;parent_slot<=4;++parent_slot){
  jewel::ShowerState angles(-1e6);angles.colour_max=101;jewel::ShowerParticle anchor;anchor.particle.status=21;
  jewel::ParticleId parents[4];for(auto& id:parents)id=angles.append(anchor,true,true);
  jewel::ShowerParticle selected;selected.particle.status=4;selected.particle.pdg=1;
  selected.particle.colour=101;selected.particle.momentum={10,0,0,10};selected.parent=parents[3];
  auto selected_id=angles.append(selected,true,true);
  jewel::ShowerParticle partner;partner.particle.status=4;partner.particle.pdg=-1;
  partner.particle.anticolour=101;partner.particle.momentum={0,10,0,10};partner.parent=parents[parent_slot-1];
  auto partner_id=angles.append(partner,true,true);angles.validate();
  auto opening=jewel::colour_opening_angle(angles,selected_id);
  check(parent_slot<=2?opening==-1:std::abs(opening-std::acos(0.0))<1e-15);
  // MPI provenance may point beyond slot 2, while source scheduling parent is 0.
  angles.at(partner_id).scheduling_slots=std::array<std::size_t,3>{0,0,0};
  check(angles.at(partner_id).parent==parents[parent_slot-1]);
  check(jewel::colour_opening_angle(angles,selected_id)==-1);
 }
 // An inactive massless MPI photon retains its identity and never showers.
 state.restore(baseline);auto& photon=state.at(first);
 photon.particle.pdg=22;photon.particle.status=4;photon.particle.colour=0;photon.particle.anticolour=0;
 auto photon_baseline=state.snapshot();
 jewel::HardInitializationConfig photon_config;photon_config.mpi_pair=true;photon_config.mpi_fsr=false;
 photon_config.mpi_sampling_channels=std::array<jewel::Channel,2>{jewel::Channel::GC,jewel::Channel::QQ};
 jewel::ReplayRandom photon_draws({.5});
 jewel::initialize_hard_dijet(state,first,second,photon_config,sampler,photon_draws);
 check(photon_draws.checkpoint()==1&&state.at(first).particle.pdg==22&&state.at(first).particle.status==4);
 check(state.at(first).record_mass==0&&state.at(first).decay_time==photon_config.lifetime&&state.at(first).fraction==0);
 for(int invalid=0;invalid<4;++invalid){
  state.restore(photon_baseline);auto cfg=photon_config;
  if(invalid==0)state.at(first).record_mass=.1;
  if(invalid==1)state.at(first).particle.status=1;
  if(invalid==2)state.at(first).particle.colour=101;
  if(invalid==3)cfg.mpi_pair=false;
  jewel::ReplayRandom untouched({});bool failed=false;
  try{jewel::initialize_hard_dijet(state,first,second,cfg,sampler,untouched);}catch(const std::domain_error&){failed=true;}
  check(failed&&untouched.checkpoint()==0);
 }

 // Native inactive-time convention retains nonparton mass and the source draw.
 state.restore(photon_baseline);state.at(first).particle.pdg=10441;state.at(first).record_mass=3.5;
 state.at(first).particle.momentum.energy=std::sqrt(100.+3.5*3.5);
 double passive_total=state.at(first).particle.momentum.energy+state.at(second).particle.momentum.energy;
 auto passive_baseline=state.snapshot();jewel::ReplayRandom strict({});bool passive_rejected=false;
 try{jewel::initialize_hard_dijet(state,first,second,photon_config,sampler,strict);}catch(const std::domain_error&){passive_rejected=true;}
 check(passive_rejected&&strict.checkpoint()==0);
 photon_config.inactive_nonparton_lifetime=true;jewel::ReplayRandom passive_draws({.5,.7});
 jewel::initialize_hard_dijet(state,first,second,photon_config,sampler,passive_draws);
 check(passive_draws.checkpoint()==2&&state.at(first).record_mass==3.5&&state.at(first).particle.status==4);
 check(state.at(first).decay_time==photon_config.lifetime&&state.at(first).fraction==0&&state.at(first).angle==0);
 check(std::abs(state.at(first).particle.momentum.energy+state.at(second).particle.momentum.energy-passive_total)<1e-12);
 state.validate();

}
