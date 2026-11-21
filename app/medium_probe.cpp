// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/scattering.hpp"
#include "jewel/geometry.hpp"
#include "jewel/branching.hpp"
#include "jewel/shower.hpp"
#include "jewel/transfer.hpp"
#include "jewel/evolution.hpp"
#include "jewel/collision.hpp"
#include "jewel/coherence.hpp"
#include "jewel/initial_evolution.hpp"
#include "jewel/cascade.hpp"
#include "jewel/hard_initialization.hpp"
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <memory>
#include <stdexcept>
int main() {
 try {
 jewel::ExponentialIntegral ei;
 std::unique_ptr<jewel::ExpandingMedium> medium;
 std::unique_ptr<jewel::InitialRadiation> radiation;
 std::unique_ptr<jewel::InitialSudakovTable> sudakov;
 std::unique_ptr<jewel::FinalSplittingTable> final_table;
 std::unique_ptr<jewel::PartonicPdf> pdf;
 std::unique_ptr<jewel::ScatteringRates> rates;
 std::unique_ptr<jewel::BranchingSampler> branching;
 std::unique_ptr<jewel::TransferSampler> transfers;
 jewel::EvolutionConfig evolution_config;jewel::Kernels evolution_kernels;
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
   if(op=="LENGTHS") {
    double x,y,b,r,t,step;jewel::Momentum p;
    if(!(std::cin>>x>>y>>b>>r>>t>>p.px>>p.py>>p.pz>>p.energy>>step))throw std::runtime_error("bad geometry query");
    auto v=jewel::straight_path_lengths(x,y,b,r,t,p,step,*medium);
    std::cout<<v.geometric_transverse<<' '<<v.hot<<' '<<v.hot_transverse<<' '<<v.temperature_cubed_integral<<'\n';
   } else if(op=="EVENTB") {
    double b;if(!(std::cin>>b))throw std::runtime_error("bad event impact parameter");
    *medium=medium->with_impact_parameter(b);
    if(rates)*rates=rates->with_event_impact_parameter(b);
    if(transfers)*transfers=transfers->with_event_impact_parameter(b);
   } else if(op=="ECONFIG") {
    std::cin>>evolution_config.cutoff>>evolution_config.mass_scale>>evolution_config.formation_factor
     >>evolution_config.lifetime>>evolution_config.cross_section_scale;
   } else if(op=="PAIR"||op=="HIT") {
    if(!branching)throw std::runtime_error("BCONFIG required");
    jewel::CollisionConfig config;double total,remaining,target,delta;int last;
    std::cin>>config.kinematic_mode>>config.cutoff>>config.mass_scale>>total>>remaining>>target>>delta>>last;
    config.last_transfer=last!=0;
    jewel::ShowerParticle leg,scratch;auto& p=leg.particle.momentum;auto& v=leg.generator_vertex;auto& x=leg.particle.production;
    std::cin>>p.px>>p.py>>p.pz>>p.energy>>leg.record_mass>>leg.particle.pdg>>leg.particle.status
     >>x.x>>x.y>>x.z>>x.t>>v.x>>v.y>>v.z>>v.t;
    auto& sv=scratch.generator_vertex;std::cin>>sv.x>>sv.y>>sv.z>>sv.t;
    jewel::TransferRecord transfer;auto& q=transfer.scatterer.momentum;auto& y=transfer.position;
    std::cin>>transfer.invariant>>transfer.scatterer.type>>q.px>>q.py>>q.pz>>q.energy>>transfer.scatterer.mass>>y.x>>y.y>>y.z>>y.t;
    jewel::CollisionFinalizationConfig finish;jewel::CollisionStepTiming timing;
    jewel::RecoilStorage ledger;int colours=0;
    if(op=="HIT"){
     int had,scat,write,subtract,qp,coherent;
     std::cin>>finish.formation_factor>>finish.soft_recoil_cut>>finish.hard_recoil_cut>>had>>scat>>finish.storage.mode>>write>>subtract;
     finish.hadronize_recoils=had;finish.scatter_recoils=scat;finish.storage.write_centres=write;finish.storage.subtract=subtract;
     std::cin>>timing.fraction>>qp>>timing.first_decay_time>>timing.coherent_end_time>>timing.next_transfer_time>>timing.transfer_time>>coherent;
     timing.quark_pair=qp;timing.coherent=coherent;timing.total_mass_change=delta;timing.last_transfer=last;
     leg.decay_time=timing.first_decay_time;
     std::cin>>leg.particle.colour>>leg.particle.anticolour>>colours>>ledger.used>>ledger.capacity;
     ledger.records.resize(ledger.capacity);for(auto& r:ledger.records)r={-99,{1,2,3,4},5,{6,7,8,9},10};
    }
    std::size_t count;if(!(std::cin>>count)||count>100000)throw std::runtime_error("bad collision tape");
    std::vector<double> tape(count);for(auto& a:tape)std::cin>>a;
    if(!std::cin)throw std::runtime_error("bad collision replay");jewel::ReplayRandom random(tape);
    jewel::CollisionPair result;
    try{result=jewel::scatter_pair(leg,scratch,transfer,total,remaining,target,delta,config,*branching,*medium,random);
     if(op=="HIT")jewel::finish_collision(result,colours,ledger,finish,timing,*medium,random);}
    catch(const std::domain_error&){
     std::cout<<"0 0 0 0 0 0 0 0 0 "<<random.checkpoint()<<" 1\n";
     for(int i=0;i<4;++i){std::cout<<"0 0 0 0 0 0 0 0 0 0 0";if(op=="HIT")std::cout<<" 0 0 0 0 0 0 0 0 0 0 0";std::cout<<'\n';}
     if(op=="HIT"){std::cout<<"0 0\n";for(std::size_t i=0;i<ledger.capacity;++i)std::cout<<"0 0 0 0 0 0 0 0 0 0 0\n";}
     continue;
    }
    std::cout<<result.mass_change<<' '<<result.remaining_transfer<<' '<<result.target_mass<<' '<<result.recoil_fraction<<' '
     <<result.split_recoil<<' '<<result.recoil_pair<<' '<<result.rejected<<' '<<result.reshuffled<<' '<<result.colour_direction<<' '<<random.checkpoint()<<" 0\n";
    for(auto a:{result.centre,result.incoming,result.recoil,result.outgoing}){
     auto p=a.particle.momentum;auto v=a.generator_vertex;
     std::cout<<a.particle.status<<' '<<a.particle.pdg<<' '<<p.px<<' '<<p.py<<' '<<p.pz<<' '<<p.energy<<' '<<a.record_mass<<' '
      <<v.x<<' '<<v.y<<' '<<v.z<<' '<<v.t;
     if(op=="HIT"){
      auto x=a.particle.production;
      std::cout<<' '<<a.particle.colour<<' '<<a.particle.anticolour<<' '<<x.x<<' '<<x.y<<' '<<x.z<<' '<<x.t<<' '<<a.decay_time
       <<' '<<a.fraction<<' '<<a.angle<<' '<<a.quark_pair<<' '<<a.angular_required;
     }
     std::cout<<'\n';
    }
    if(op=="HIT"){
     std::cout<<colours<<' '<<ledger.used<<'\n';
     for(auto r:ledger.records)std::cout<<r.flavour<<' '<<r.centre.px<<' '<<r.centre.py<<' '<<r.centre.pz<<' '<<r.centre.energy<<' '<<r.centre_mass<<' '
      <<r.dummy.px<<' '<<r.dummy.py<<' '<<r.dummy.pz<<' '<<r.dummy.energy<<' '<<r.dummy_mass<<'\n';
    }
   } else if(op=="SEQUENCE") {
    if(!branching)throw std::runtime_error("BCONFIG required");
    jewel::CollisionConfig config;jewel::CollisionSequenceRequest request;int qp,coh;std::size_t first,last;
    std::cin>>config.kinematic_mode>>config.cutoff>>config.mass_scale>>request.target_mass>>request.time>>request.energy_fraction
     >>request.fraction>>qp>>coh>>first>>last;
    if(first<1||first>last||last>10000)throw std::runtime_error("bad sequence range");
    request.first=first-1;request.last=last-1;request.quark_pair=qp;request.coherent=coh;
    jewel::ShowerState state;jewel::ShowerParticle leg,scratch;auto& p=leg.particle.momentum;auto& v=leg.generator_vertex;auto& x=leg.particle.production;
    std::cin>>p.px>>p.py>>p.pz>>p.energy>>leg.record_mass>>leg.particle.pdg>>leg.particle.status
     >>x.x>>x.y>>x.z>>x.t>>v.x>>v.y>>v.z>>v.t>>leg.decay_time>>leg.fraction>>leg.angle
     >>leg.particle.colour>>leg.particle.anticolour>>state.colour_max;
    auto scratch_id=state.append(scratch),id=state.append(leg);
    jewel::CollisionFinalizationConfig finish;jewel::RecoilStorage ledger;int had,scat,write,subtract;
    std::cin>>finish.formation_factor>>finish.soft_recoil_cut>>finish.hard_recoil_cut>>had>>scat>>finish.storage.mode>>write>>subtract>>ledger.used>>ledger.capacity;
    finish.hadronize_recoils=had;finish.scatter_recoils=scat;finish.storage.write_centres=write;finish.storage.subtract=subtract;
    ledger.records.resize(ledger.capacity);for(auto& r:ledger.records)r={-99,{1,2,3,4},5,{6,7,8,9},10};
    jewel::TransferHistory history;history.records.resize(last);
    for(auto& r:history.records){auto& q=r.scatterer.momentum;auto& y=r.position;
     std::cin>>r.invariant>>r.time>>r.scatterer.type>>q.px>>q.py>>q.pz>>q.energy>>r.scatterer.mass>>y.x>>y.y>>y.z>>y.t;
    }
    std::size_t count;if(!(std::cin>>count)||count>100000)throw std::runtime_error("bad sequence tape");
    std::vector<double> tape(count);for(auto& a:tape)std::cin>>a;
    if(!std::cin)throw std::runtime_error("bad sequence input");jewel::ReplayRandom random(tape);
    jewel::CollisionSequenceResult result{id,request.last,0,request.fraction,false};int invalid=0;
    try{result=jewel::apply_collisions(state,id,scratch_id,history,ledger,request,config,finish,*branching,*medium,random);state.validate();}
    catch(const std::domain_error&){invalid=1;}
    catch(const std::out_of_range& e){if(std::string(e.what())!="RNG tape exhausted")throw;invalid=3;}
    catch(const std::runtime_error& e){if(std::string(e.what())!="collision sequence retries exhausted")throw;invalid=2;}
    auto slot=[&](jewel::ParticleId id){if(id==0)return std::size_t(0);for(std::size_t i=0;i<state.particles().size();++i)if(state.particles()[i].particle.id==id)return i+1;throw std::runtime_error("dangling sequence ID");};
    std::cout<<state.particles().size()<<' '<<state.colour_max<<' '<<ledger.used<<' '<<result.retry_splitting<<' '
     <<result.last+1<<' '<<result.fraction<<' '<<random.checkpoint()<<' '<<state.discarded<<' '<<invalid<<'\n';
    for(auto a:state.particles()){
     auto p=a.particle.momentum;auto v=a.generator_vertex;auto x=a.particle.production;
     std::cout<<a.particle.status<<' '<<a.particle.pdg<<' '<<slot(a.parent)<<' '<<slot(a.daughters[0])<<' '<<slot(a.daughters[1])<<' '
      <<p.px<<' '<<p.py<<' '<<p.pz<<' '<<p.energy<<' '<<a.record_mass<<' '<<v.x<<' '<<v.y<<' '<<v.z<<' '<<v.t<<' '
      <<x.x<<' '<<x.y<<' '<<x.z<<' '<<x.t<<' '<<a.decay_time<<' '<<a.fraction<<' '<<a.angle<<' '<<a.quark_pair<<' '
      <<a.angular_required<<' '<<a.particle.colour<<' '<<a.particle.anticolour<<'\n';
    }
    for(auto r:ledger.records)std::cout<<r.flavour<<' '<<r.centre.px<<' '<<r.centre.py<<' '<<r.centre.pz<<' '<<r.centre.energy<<' '<<r.centre_mass<<' '
     <<r.dummy.px<<' '<<r.dummy.py<<' '<<r.dummy.pz<<' '<<r.dummy.energy<<' '<<r.dummy_mass<<'\n';
   } else if(op=="COHERENT"||op=="ISTATE"||op=="BRANCH"||op=="CASCADE"||op=="GCASCADE"||op=="CCASCADE"||op=="NCASCADE") {
    if(!rates||!transfers||!branching)throw std::runtime_error("coherent models required");
    jewel::CoherentConfig config;config.lifetime=evolution_config.lifetime;config.cross_section_scale=evolution_config.cross_section_scale;
    config.collision.cutoff=evolution_config.cutoff;config.collision.mass_scale=evolution_config.mass_scale;
    int enabled;std::size_t selected;double now,left,angle,split_time;
    jewel::ShowerState state((op=="BRANCH"||op=="CASCADE"||op=="GCASCADE"||op=="CCASCADE"||op=="NCASCADE")?-1e6:0);jewel::TransferHistory history;history.records.resize(100);
    std::cin>>selected>>enabled>>now>>left>>angle>>split_time>>config.length_factor>>config.collision.kinematic_mode>>state.colour_max>>state.weight>>history.last;
    config.enabled=enabled;history.first=1;int retry_initial=0;double coherence_rejection=1;
    if(op!="COHERENT")std::cin>>retry_initial>>coherence_rejection;
    std::size_t input_count=5;if(op=="NCASCADE")std::cin>>input_count;
    if(input_count<2||input_count>22990)throw std::runtime_error("invalid native record count");
    std::vector<std::array<jewel::ParticleId,2>> links(input_count);
    for(std::size_t i=0;i<input_count;++i){jewel::ShowerParticle s;auto& p=s.particle;int qp,ao;
     std::cin>>p.status>>p.pdg>>s.parent>>links[i][0]>>links[i][1]>>p.momentum.px>>p.momentum.py>>p.momentum.pz>>p.momentum.energy>>s.record_mass
      >>s.generator_vertex.x>>s.generator_vertex.y>>s.generator_vertex.z>>s.generator_vertex.t>>p.production.x>>p.production.y>>p.production.z>>p.production.t>>s.decay_time
      >>s.fraction>>s.angle>>qp>>ao>>p.colour>>p.anticolour>>s.evolution_time;
     s.quark_pair=qp;s.angular_required=ao;state.append(s);
    }
    for(std::size_t i=0;i<input_count;++i)state.at(i+1).daughters=links[i];
    if(op=="CCASCADE"){
     std::size_t padding;std::cin>>padding;if(padding>22995)throw std::runtime_error("invalid compression padding");
     jewel::ShowerParticle retired;retired.particle.status=13;
     for(std::size_t i=0;i<padding;++i)state.append(retired,true,true);
    }
    auto& finish=config.finalization;jewel::RecoilStorage ledger;int had,scat,write,subtract;
    std::cin>>finish.formation_factor>>finish.soft_recoil_cut>>finish.hard_recoil_cut>>had>>scat>>finish.storage.mode>>write>>subtract>>ledger.capacity;
    finish.hadronize_recoils=had;finish.scatter_recoils=scat;finish.storage.write_centres=write;finish.storage.subtract=subtract;
    ledger.records.resize(ledger.capacity);for(auto& r:ledger.records)r={-99,{1,2,3,4},5,{6,7,8,9},10};
    if(history.last>99)throw std::runtime_error("bad coherent history");
    for(std::size_t i=0;i<history.last;++i){auto& r=history.records[i];auto& q=r.momentum;auto& a=r.scatterer.momentum;auto& x=r.position;
     std::cin>>r.invariant>>q.px>>q.py>>q.pz>>q.energy>>r.time>>r.scatterer.type>>a.px>>a.py>>a.pz>>a.energy>>r.scatterer.mass>>x.x>>x.y>>x.z>>x.t;
     history.sum.px+=q.px;history.sum.py+=q.py;history.sum.pz+=q.pz;history.sum.energy+=q.energy;history.sum_squared+=r.invariant;
    }
    std::int64_t count;std::cin>>count;if(count < -1||count>2000000)throw std::runtime_error("bad coherent tape");
    std::uint64_t seed=0;std::vector<double> tape;
    if(count==-1)std::cin>>seed;else{tape.resize(count);for(auto& v:tape)std::cin>>v;}
    if(!std::cin)throw std::runtime_error("bad coherent input");
    jewel::ReplayRandom replay(std::move(tape));jewel::SplitMixRandom generated(seed);
    if(op=="NCASCADE"){std::uint64_t offset;std::cin>>offset;if(!std::cin||count!=-1)throw std::runtime_error("invalid native random offset");generated.restore(offset);}
    jewel::Random& random=count==-1?static_cast<jewel::Random&>(generated):static_cast<jewel::Random&>(replay);
    auto checkpoint=[&]{return count==-1?generated.checkpoint():replay.checkpoint();};jewel::CoherentHistograms histograms;jewel::CoherentResult result;result.selected=selected;int invalid=0;
    jewel::InitialScatteringResult initial_result;initial_result.selected=selected;
    try{
     if(op!="COHERENT"){
      if(!pdf||!sudakov||!final_table)throw std::runtime_error("initial models required");
      jewel::InitialEvolutionConfig initial_config{evolution_config,config,coherence_rejection};
      jewel::InitialScatteringEvolution evolution(initial_config,evolution_kernels,*branching,*sudakov,*final_table,*pdf,*rates,*transfers,*medium);
      if(op=="ISTATE"){
       initial_result=evolution.sample(state,selected,1,history,ledger,now,left,retry_initial,random,&histograms);result.selected=initial_result.selected;
      }else{
       jewel::ScatteringEvolution final_evolution(evolution_config,*branching,*sudakov,*final_table,*rates,*transfers);
       jewel::Cascade cascade({evolution_config,config.collision,config.finalization,1000000,op=="CCASCADE"||op=="NCASCADE"},evolution,final_evolution,*branching,*medium);
       if(op=="BRANCH")cascade.branch(state,selected,1,history,ledger,random,&histograms);
       else {
        jewel::CascadeObserver observer;
        const char* trace=std::getenv("JEWEL_CASCADE_TRACE");
        if(trace&&std::string(trace)=="1")observer=[&](std::size_t call,std::size_t index,bool before,const jewel::ShowerState& current){
         std::cerr<<std::setprecision(17)<<"CASCADE_TRACE "<<(before?0:1)<<' '<<call<<' '<<index+1<<' '<<current.particles().size()<<' '<<checkpoint()<<' '<<current.discarded;
         if(before){const auto& s=current.particles().at(index);const auto& p=s.particle.momentum;
          std::cerr<<' '<<p.px<<' '<<p.py<<' '<<p.pz<<' '<<p.energy<<' '<<s.record_mass<<' '<<s.fraction<<' '<<s.evolution_time<<' '<<s.particle.production.t<<' '<<s.decay_time;}
         std::cerr<<'\n';
        };
        cascade.evolve(state,1,history,ledger,random,&histograms,observer);
       }
      }
     }else result=jewel::evolve_coherent(state,selected,1,history,ledger,now,left,angle,split_time,config,*rates,*transfers,*branching,*medium,random,&histograms);
     state.validate();
    }
    catch(const std::domain_error& e){invalid=1;std::cerr<<(op=="ISTATE"?"INI_ERROR ":"COH_ERROR ")<<e.what()<<'\n';}
    catch(const std::out_of_range& e){if(std::string(e.what())!="RNG tape exhausted")throw;invalid=3;}
    if(op=="ISTATE")std::cerr<<"INI_STATS "<<initial_result.trials<<' '<<initial_result.unresolved_rejections<<' '<<initial_result.coherence_reversions<<'\n';
    else std::cerr<<"COH_STATS "<<result.collision_calls<<' '<<result.kinematic_calls<<' '<<result.undone_kicks<<'\n';
    auto slot=[&](jewel::ParticleId id){if(!id)return std::size_t(0);for(std::size_t i=0;i<state.particles().size();++i)if(state.particles()[i].particle.id==id)return i+1;throw std::runtime_error("dangling coherent ID");};
    std::cout<<state.particles().size()<<' '<<state.colour_max<<' '<<ledger.used<<' '<<(op=="CCASCADE"?selected:slot(result.selected))<<' '<<checkpoint()<<' '<<state.discarded<<' '<<invalid<<' '<<history.last;
    if(op=="ISTATE")std::cout<<' '<<initial_result.fraction<<' '<<initial_result.virtuality<<' '<<initial_result.elapsed<<' '<<initial_result.incoming_flavour<<' '<<initial_result.above_cutoff<<' '<<bool(initial_result.coherent);
    std::cout<<'\n';
    for(const auto& s:state.particles()){auto& p=s.particle;std::cout<<p.status<<' '<<p.pdg<<' '<<slot(state.scheduling_parent(p.id))<<' '<<slot(state.scheduling_daughters(p.id)[0])<<' '<<slot(state.scheduling_daughters(p.id)[1])<<' '
     <<p.momentum.px<<' '<<p.momentum.py<<' '<<p.momentum.pz<<' '<<p.momentum.energy<<' '<<s.record_mass<<' '
     <<s.generator_vertex.x<<' '<<s.generator_vertex.y<<' '<<s.generator_vertex.z<<' '<<s.generator_vertex.t<<' '
     <<p.production.x<<' '<<p.production.y<<' '<<p.production.z<<' '<<p.production.t<<' '<<s.decay_time<<' '<<s.fraction<<' '<<s.angle<<' '
     <<s.quark_pair<<' '<<s.angular_required<<' '<<p.colour<<' '<<p.anticolour<<' '<<s.evolution_time;
     if(op=="BRANCH"||op=="CASCADE"||op=="GCASCADE"||op=="CCASCADE"||op=="NCASCADE")std::cout<<' '<<s.medium_induced;
     std::cout<<'\n';}
    std::cout<<history.sum.px<<' '<<history.sum.py<<' '<<history.sum.pz<<' '<<history.sum.energy<<' '<<history.sum_squared<<'\n';
    for(const auto& r:history.records)std::cout<<r.invariant<<' '<<r.momentum.px<<' '<<r.momentum.py<<' '<<r.momentum.pz<<' '<<r.momentum.energy<<' '<<r.time<<' '
     <<r.scatterer.type<<' '<<r.scatterer.momentum.px<<' '<<r.scatterer.momentum.py<<' '<<r.scatterer.momentum.pz<<' '<<r.scatterer.momentum.energy<<' '<<r.scatterer.mass<<' '
     <<r.position.x<<' '<<r.position.y<<' '<<r.position.z<<' '<<r.position.t<<'\n';
    for(const auto& r:ledger.records)std::cout<<r.flavour<<' '<<r.centre.px<<' '<<r.centre.py<<' '<<r.centre.pz<<' '<<r.centre.energy<<' '<<r.centre_mass<<' '
     <<r.dummy.px<<' '<<r.dummy.py<<' '<<r.dummy.pz<<' '<<r.dummy.energy<<' '<<r.dummy_mass<<'\n';
    for(const auto& h:{histograms.time,histograms.transfer}){for(auto value:h.values)std::cout<<value<<' ';std::cout<<'\n';}
   } else if(op=="FSTATE") {
    if(!branching||!sudakov||!final_table||!rates||!transfers)throw std::runtime_error("evolution models required");
    double now,left;std::cin>>now>>left;
    jewel::ShowerState state;jewel::ShowerParticle temp,leg;temp.particle.status=13;leg.particle.status=1;
    auto& p=leg.particle.momentum;auto& v=leg.generator_vertex;auto& birth=leg.particle.production;
    std::cin>>p.px>>p.py>>p.pz>>p.energy>>leg.record_mass>>leg.particle.pdg>>birth.x>>birth.y>>birth.z>>birth.t>>v.x>>v.y>>v.z>>v.t;
    auto scratch=state.append(temp),id=state.append(leg);
    jewel::FinalScatteringResult input;int pair,above;
    std::cin>>input.mass>>input.fraction>>pair>>above;input.quark_pair=pair!=0;input.above_cutoff=above!=0;
    jewel::TransferHistory history;history.records.resize(100);auto& sum=history.sum;
    std::cin>>history.first>>history.last>>sum.px>>sum.py>>sum.pz>>sum.energy>>history.sum_squared;
    if(history.last>100)throw std::runtime_error("invalid initial transfer count");
    for(std::size_t i=0;i<history.last;++i){
     auto& r=history.records[i];auto& q=r.momentum;auto& a=r.scatterer.momentum;auto& x=r.position;
     std::cin>>r.invariant>>q.px>>q.py>>q.pz>>q.energy>>r.time>>r.scatterer.type
      >>a.px>>a.py>>a.pz>>a.energy>>r.scatterer.mass>>x.x>>x.y>>x.z>>x.t;
    }
    std::size_t count;if(!(std::cin>>count)||count>100000)throw std::runtime_error("bad evolution replay");
    std::vector<double> tape(count);for(auto& x:tape)if(!(std::cin>>x))throw std::runtime_error("short replay");
    jewel::ReplayRandom random(tape);
    jewel::ScatteringEvolution evolution(evolution_config,*branching,*sudakov,*final_table,*rates,*transfers);
    auto result=evolution.final_state(state,id,scratch,history,now,left,input,random);
    auto momentum=[](jewel::Momentum p){std::cout<<p.px<<' '<<p.py<<' '<<p.pz<<' '<<p.energy<<' ';};
    auto position=[](jewel::SpaceTime x){std::cout<<x.x<<' '<<x.y<<' '<<x.z<<' '<<x.t<<' ';};
    std::cout<<result.mass<<' '<<result.elapsed<<' '<<result.fraction<<' '<<int(result.quark_pair)<<' '
     <<int(result.above_cutoff)<<' '<<history.first<<' '<<history.last<<' '<<random.checkpoint()<<'\n';
    momentum(history.sum);std::cout<<history.sum_squared<<'\n';
    const auto& out=state.at(id);momentum(out.particle.momentum);std::cout<<out.record_mass<<' ';position(out.generator_vertex);std::cout<<'\n';
    const auto& tmp=state.at(scratch);std::cout<<tmp.particle.status<<' '<<tmp.particle.pdg<<' ';momentum(tmp.particle.momentum);
    std::cout<<tmp.record_mass<<' ';position(tmp.generator_vertex);position(tmp.particle.production);std::cout<<'\n';
    for(const auto& r:history.records){std::cout<<r.invariant<<' ';momentum(r.momentum);std::cout<<r.time<<' '<<r.scatterer.type<<' ';
     momentum(r.scatterer.momentum);std::cout<<r.scatterer.mass<<' ';position(r.position);std::cout<<'\n';}
   } else if(op=="QCONFIG") {
    jewel::Kernels k;int boost;std::cin>>k.nf>>k.lambda_shower>>k.lambda_scattering>>boost;
    transfers=std::make_unique<jewel::TransferSampler>(k,*medium,boost!=0);
   } else if(op=="TRANSFER") {
    if(!transfers)throw std::runtime_error("QCONFIG required");
    std::size_t slot;int recalc;double dt,x;std::cin>>slot>>recalc>>dt>>x;
    if(slot==0||slot>10000)throw std::runtime_error("invalid transfer slot");
    jewel::ShowerState state;jewel::ShowerParticle temporary,leg;leg.particle.status=1;leg.particle.pdg=21;
    auto& p=leg.particle.momentum;auto& v=leg.generator_vertex;auto& birth=leg.particle.production;
    std::cin>>p.px>>p.py>>p.pz>>p.energy>>leg.record_mass>>birth.x>>birth.y>>birth.z>>birth.t>>v.x>>v.y>>v.z>>v.t;
    auto& tv=temporary.generator_vertex;std::cin>>tv.x>>tv.y>>tv.z>>tv.t;
    auto scratch=state.append(temporary),projectile=state.append(leg);
    jewel::TransferHistory history;history.records.resize(slot);auto& sum=history.sum;
    std::cin>>sum.px>>sum.py>>sum.pz>>sum.energy>>history.sum_squared;
    auto& record=history.records[slot-1];auto& q=record.momentum;auto& a=record.scatterer.momentum;auto& r=record.position;
    std::cin>>record.invariant>>q.px>>q.py>>q.pz>>q.energy>>record.time>>record.scatterer.type
     >>a.px>>a.py>>a.pz>>a.energy>>record.scatterer.mass>>r.x>>r.y>>r.z>>r.t;
    std::size_t count;if(!(std::cin>>count)||count>100000)throw std::runtime_error("invalid transfer replay");
    std::vector<double> tape(count);for(auto& value:tape)if(!(std::cin>>value))throw std::runtime_error("short replay");
    jewel::ReplayRandom random(tape);transfers->sample(state,projectile,scratch,history,slot-1,dt,x,recalc!=0,random);
    auto output=[](jewel::Momentum p){std::cout<<p.px<<' '<<p.py<<' '<<p.pz<<' '<<p.energy<<' ';};
    auto position=[](jewel::SpaceTime p){std::cout<<p.x<<' '<<p.y<<' '<<p.z<<' '<<p.t<<' ';};
    const auto& temp=state.at(scratch);const auto& result=state.at(projectile);
    std::cout<<int(state.discarded)<<' '<<random.checkpoint()<<' '<<temp.particle.status<<' '<<temp.particle.pdg<<'\n';
    std::cout<<record.invariant<<' ';output(record.momentum);std::cout<<record.time<<'\n';
    std::cout<<record.scatterer.type<<' ';output(record.scatterer.momentum);std::cout<<record.scatterer.mass<<' ';position(record.position);std::cout<<'\n';
    output(history.sum);std::cout<<history.sum_squared<<'\n';
    output(result.particle.momentum);std::cout<<result.record_mass<<' ';position(result.generator_vertex);std::cout<<'\n';
    output(temp.particle.momentum);std::cout<<temp.record_mass<<' ';position(temp.generator_vertex);position(temp.particle.production);std::cout<<'\n';
   } else if(op=="NEWMASS") {
    if(!branching||!sudakov||!final_table)throw std::runtime_error("BCONFIG, RCONFIG and FCONFIG required");
    jewel::VirtualityUpdate u;int initial,pair;
    std::cin>>u.q_squared>>u.old_q_squared>>u.previous_mass>>u.record_mass>>u.energy>>u.pdg>>initial
     >>u.energy_fraction>>u.previous_fraction>>pair;
    u.initial_state=initial!=0;u.previous_pair=pair!=0;
    std::size_t count;if(!(std::cin>>count)||count>100000)throw std::runtime_error("invalid update replay");
    std::vector<double> tape(count);for(auto& x:tape)if(!(std::cin>>x))throw std::runtime_error("short replay");
    jewel::ReplayRandom random(tape);
    auto value=branching->updated_mass(u,*sudakov,*final_table,random);
    std::cout<<value.mass<<' '<<value.fraction.value()<<' '<<int(value.quark_pair.value())<<' '<<random.checkpoint()<<'\n';
   } else if(op=="NORAD") {
    if(!rates)throw std::runtime_error("XCONFIG required");
    jewel::Momentum p;jewel::SpaceTime v;double mass;int pdg;
    if(!(std::cin>>p.px>>p.py>>p.pz>>p.energy>>mass>>pdg>>v.x>>v.y>>v.z>>v.t))throw std::runtime_error("invalid radiation query");
    std::cout<<rates->no_initial_radiation(p,mass,pdg,v)<<'\n';
   } else if(op=="REJECT"||op=="SPLIT"||op=="ISPLIT") {
    if(!branching)throw std::runtime_error("BCONFIG required");
    std::size_t n; jewel::ParticleId mother;std::cin>>n>>mother;
    if(n>23000||n==0)throw std::runtime_error("invalid event size");
    jewel::ShowerState state;double formation=1;
    if(op!="REJECT")std::cin>>formation>>state.colour_max;
    jewel::InitialSplitRequest initial;bool invalid=false;
    if(op=="ISPLIT"){
     initial.formation_factor=formation;
     std::cin>>initial.fraction>>initial.transfer_sum>>initial.virtuality>>initial.incoming_flavour>>initial.time>>initial.radiation_time>>initial.mass_scale;
    }
    std::vector<std::array<jewel::ParticleId,2>> links;
    for(std::size_t i=0;i<n;++i){
     jewel::ShowerParticle a;std::array<jewel::ParticleId,2> daughters;auto& p=a.particle.momentum;auto& v=a.generator_vertex;auto& x=a.particle.production;int qq,ao;
     std::cin>>a.particle.status>>a.particle.pdg>>a.parent>>daughters[0]>>daughters[1]
      >>p.px>>p.py>>p.pz>>p.energy>>a.record_mass>>v.x>>v.y>>v.z>>v.t
      >>x.x>>x.y>>x.z>>x.t>>a.decay_time>>a.evolution_time>>a.fraction>>a.angle>>qq>>ao;
     if(op!="REJECT")std::cin>>a.particle.colour>>a.particle.anticolour;
     if(!std::cin)throw std::runtime_error("bad shower record");
     a.quark_pair=qq!=0;a.angular_required=ao!=0;state.append(a);links.push_back(daughters);
    }
    for(std::size_t i=0;i<n;++i)state.at(i+1).daughters=links[i];
    state.validate();std::size_t count;std::cin>>count;
    if(count>100000)throw std::runtime_error("bad replay size");
    std::vector<double> tape(count);for(auto& x:tape)if(!(std::cin>>x))throw std::runtime_error("short replay");
    jewel::ReplayRandom random(tape);
    if(op=="ISPLIT"){
     try{initial.fraction=jewel::make_initial_splitting(state,mother,initial,*branching,random).fraction;}
     catch(const std::domain_error&){invalid=true;}
    }
    else if(op=="SPLIT")jewel::make_splitting(state,mother,*branching,*medium,formation,random);
    else jewel::reject_splitting(state,mother,*branching,random);
    if(!invalid)state.validate();
    std::cout<<int(state.discarded)<<' '<<random.checkpoint();
    if(op!="REJECT")std::cout<<' '<<state.particles().size()<<' '<<state.colour_max;
    if(op=="ISPLIT")std::cout<<' '<<initial.fraction<<' '<<invalid;
    std::cout<<'\n';
    for(const auto& a:state.particles()){
     auto p=a.particle.momentum;auto v=a.generator_vertex;auto x=a.particle.production;
     std::cout<<a.particle.status<<' '<<a.particle.pdg<<' '<<a.parent<<' '<<a.daughters[0]<<' '<<a.daughters[1]<<' '
      <<p.px<<' '<<p.py<<' '<<p.pz<<' '<<p.energy<<' '<<a.record_mass<<' '
      <<v.x<<' '<<v.y<<' '<<v.z<<' '<<v.t<<' '<<x.x<<' '<<x.y<<' '<<x.z<<' '<<x.t<<' '
      <<a.decay_time<<' '<<a.evolution_time<<' '<<a.fraction<<' '<<a.angle<<' '
      <<int(a.quark_pair)<<' '<<int(a.angular_required);
     if(op!="REJECT")std::cout<<' '<<a.particle.colour<<' '<<a.particle.anticolour;
     std::cout<<'\n';
    }
   } else if(op=="TIME") {
    if(!rates)throw std::runtime_error("XCONFIG required");
    jewel::Momentum p;jewel::SpaceTime v;double mass,start,dt,lifetime,scale;int pdg,elastic;
    if(!(std::cin>>p.px>>p.py>>p.pz>>p.energy>>mass>>pdg>>v.x>>v.y>>v.z>>v.t>>start>>dt>>lifetime>>scale>>elastic))throw std::runtime_error("bad transport query");
    std::size_t count;if(!(std::cin>>count)||count>100000)throw std::runtime_error("bad transport replay");
    std::vector<double> tape(count);for(auto& x:tape)if(!(std::cin>>x))throw std::runtime_error("short replay");
    jewel::ReplayRandom random(tape);
    auto value=rates->next_collision(p,mass,pdg,v,start,dt,lifetime,scale,elastic!=0,random);
    std::cout<<int(value.collision)<<' '<<value.elapsed<<' '<<random.checkpoint()<<'\n';
   } else if(op=="HARDINIT"||op=="MPIINIT") {
    if(!branching)throw std::runtime_error("BCONFIG required");
    jewel::HardInitializationConfig config;int ao;
    std::cin>>config.pt_hat>>config.x>>config.y>>config.formation_factor>>config.lifetime>>ao;config.first_angle_ordering=ao;
    if(op=="MPIINIT"){int fsr,channel1,channel2;std::cin>>fsr>>channel1>>channel2;config.mpi_pair=true;config.mpi_fsr=fsr;
     if((channel1!=0&&channel1!=1)||(channel2!=0&&channel2!=1))throw std::runtime_error("MPI channel must be QQ(0) or GC(1)");
     config.mpi_sampling_channels=std::array<jewel::Channel,2>{channel1?jewel::Channel::GC:jewel::Channel::QQ,channel2?jewel::Channel::GC:jewel::Channel::QQ};}
    jewel::ShowerState state(-1e6);state.colour_max=1000;
    std::array<std::array<jewel::ParticleId,2>,6> links;
    for(int i=0;i<6;++i){jewel::ShowerParticle s;auto& p=s.particle;int qp,order;
     std::cin>>p.status>>p.pdg>>s.parent>>links[i][0]>>links[i][1]>>p.momentum.px>>p.momentum.py>>p.momentum.pz>>p.momentum.energy>>s.record_mass
      >>s.generator_vertex.x>>s.generator_vertex.y>>s.generator_vertex.z>>s.generator_vertex.t>>p.production.x>>p.production.y>>p.production.z>>p.production.t>>s.decay_time
      >>s.fraction>>s.angle>>qp>>order>>p.colour>>p.anticolour>>s.evolution_time;
     s.quark_pair=qp;s.angular_required=order;state.append(s);
    }
    for(int i=0;i<6;++i)state.at(i+1).daughters=links[i];
    std::int64_t count;std::uint64_t seed=0;std::cin>>count;if(count< -1||count>2000000)throw std::runtime_error("bad hard-initialization tape");
    std::vector<double> tape;if(count==-1)std::cin>>seed;else{tape.resize(count);for(auto& u:tape)std::cin>>u;}
    if(!std::cin)throw std::runtime_error("incomplete hard-initialization input");
    jewel::ReplayRandom replay(tape);jewel::SplitMixRandom generated(seed);jewel::Random& random=count==-1?static_cast<jewel::Random&>(generated):static_cast<jewel::Random&>(replay);
    auto result=jewel::initialize_hard_dijet(state,4,5,config,*branching,random);
    std::cout<<(count==-1?generated.checkpoint():replay.checkpoint())<<' '<<result.colour_angles[0]<<' '<<result.colour_angles[1]<<'\n';
    std::cerr<<"HARD_INIT_TRIALS "<<result.trials<<'\n';
    for(const auto& s:state.particles()){const auto& p=s.particle;
     std::cout<<p.status<<' '<<p.pdg<<' '<<s.parent<<' '<<s.daughters[0]<<' '<<s.daughters[1]<<' '
      <<p.momentum.px<<' '<<p.momentum.py<<' '<<p.momentum.pz<<' '<<p.momentum.energy<<' '<<s.record_mass<<' '
      <<s.generator_vertex.x<<' '<<s.generator_vertex.y<<' '<<s.generator_vertex.z<<' '<<s.generator_vertex.t<<' '
      <<p.production.x<<' '<<p.production.y<<' '<<p.production.z<<' '<<p.production.t<<' '<<s.decay_time<<' '
      <<s.fraction<<' '<<s.angle<<' '<<s.quark_pair<<' '<<s.angular_required<<' '<<p.colour<<' '<<p.anticolour<<' '<<s.evolution_time<<'\n';
    }
   } else if(op=="BCONFIG") {
    jewel::Kernels k;double q0,pt;int angular;
    if(!(std::cin>>k.nf>>q0>>k.lambda_shower>>pt>>angular))throw std::runtime_error("invalid branching config");
    branching=std::make_unique<jewel::BranchingSampler>(k,q0,pt,angular);
   } else if(op=="MASS"||op=="FRACTION") {
    if(!branching)throw std::runtime_error("BCONFIG required");
    std::string name;double a,b,c,energy=0,maxmass=0;int initial=0;
    std::cin>>name>>a>>b>>c;if(op=="MASS")std::cin>>energy>>maxmass>>initial;
    std::size_t count;if(!(std::cin>>count)||count>100000)throw std::runtime_error("invalid replay length");
    std::vector<double> tape(count);for(auto& x:tape)if(!(std::cin>>x))throw std::runtime_error("short replay");
    jewel::ReplayRandom random(tape);jewel::Channel channel;
    if(name=="QQ")channel=jewel::Channel::QQ;else if(name=="GC")channel=jewel::Channel::GC;
    else if(name=="GG")channel=jewel::Channel::GG;else if(name=="QG")channel=jewel::Channel::QG;else if(name=="GQ")channel=jewel::Channel::GQ;else throw std::runtime_error("invalid branching channel");
    if(op=="MASS") {
     auto result=branching->mass(a,b,c,energy,channel,maxmass,initial!=0,random);
     std::cout<<result.mass<<' '<<result.fraction.value_or(-1)<<' '<<int(result.quark_pair.value_or(false))<<' '<<random.checkpoint()<<'\n';
    } else std::cout<<branching->fraction(a,b,c,channel,random)<<' '<<random.checkpoint()<<'\n';
   } else if(op=="XCONFIG") {
    jewel::Kernels k;double q0,emax,scale;int boost;
    if(!(std::cin>>k.nf>>q0>>k.lambda_shower>>k.lambda_scattering>>emax>>scale>>boost))throw std::runtime_error("bad rate configuration");
    evolution_kernels=k;rates=std::make_unique<jewel::ScatteringRates>(k,q0,emax,scale,*medium,boost!=0);
   } else if(op=="XDUMP") {
    if(!rates)throw std::runtime_error("XCONFIG required");
    std::string filename;std::getline(std::cin>>std::ws,filename);std::ofstream out(filename,std::ios::binary);
    for(double x:rates->transfers())out.write(reinterpret_cast<const char*>(&x),sizeof(x));
    for(double x:rates->debye_masses())out.write(reinterpret_cast<const char*>(&x),sizeof(x));
    for(auto row:rates->values())for(double x:row)out.write(reinterpret_cast<const char*>(&x),sizeof(x));
    if(!out)throw std::runtime_error("rate dump failed");
   } else if(op=="XINT") {
    if(!rates)throw std::runtime_error("XCONFIG required");
    std::string name;double t,md;if(!(std::cin>>name>>t>>md))throw std::runtime_error("bad rate query");
    jewel::RateChannel c;
    if(name=="QA")c=jewel::RateChannel::QA;else if(name=="QB")c=jewel::RateChannel::QB;
    else if(name=="GA")c=jewel::RateChannel::GA;else if(name=="GB")c=jewel::RateChannel::GB;else throw std::runtime_error("bad rate channel");
    std::cout<<rates->integral(t,md,c)<<'\n';
   } else if(op=="XSEC") {
    if(!rates)throw std::runtime_error("XCONFIG required");
    jewel::Momentum p;jewel::SpaceTime v;double mass,lower;char incoming,outgoing;int mode;
    if(!(std::cin>>p.px>>p.py>>p.pz>>p.energy>>mass>>lower>>incoming>>outgoing>>v.x>>v.y>>v.z>>v.t>>mode))throw std::runtime_error("bad cross section query");
    std::cout<<rates->cross_section(p,mass,lower,incoming,outgoing,v,mode)<<'\n';
   } else if(op=="PCONFIG") {
    jewel::Kernels k;double q0,emax,scale;int boost;
    if(!(std::cin>>k.nf>>q0>>k.lambda_shower>>emax>>scale>>boost))throw std::runtime_error("bad partonic PDF config");
    pdf=std::make_unique<jewel::PartonicPdf>(k,q0,emax,scale,medium->maximum_scatterer(boost!=0));
   } else if(op=="PTABLE") {
    if(!pdf)throw std::runtime_error("PCONFIG required");
    for(auto row:pdf->table()){for(double x:row)std::cout<<x<<' ';std::cout<<'\n';}
   } else if(op=="PDF"||op=="PEXACT"||op=="PINT") {
    if(!pdf)throw std::runtime_error("PCONFIG required");
    std::string name;double x,y;if(!(std::cin>>name>>x>>y))throw std::runtime_error("bad PDF query");
    jewel::Channel c;
    if(name=="QQ")c=jewel::Channel::QQ;else if(name=="QG")c=jewel::Channel::QG;
    else if(name=="GQ")c=jewel::Channel::GQ;else if(name=="GG")c=jewel::Channel::GG;else throw std::runtime_error("invalid PDF channel");
    std::cout<<(op=="PDF"?pdf->density(x,y,c):op=="PEXACT"?pdf->integrated_exact(x,c):pdf->integrated(x,c))<<'\n';
   } else if(op=="FCONFIG") {
    jewel::Kernels k;double q0,emax,scale;int boost;
    if(!(std::cin>>k.nf>>q0>>k.lambda_shower>>emax>>scale>>boost))throw std::runtime_error("bad final splitting config");
    final_table=std::make_unique<jewel::FinalSplittingTable>(k,q0,emax,scale,medium->maximum_scatterer(boost!=0));
   } else if(op=="FDUMP") {
    if(!final_table)throw std::runtime_error("FCONFIG required");
    std::string filename;std::getline(std::cin>>std::ws,filename);std::ofstream out(filename,std::ios::binary);
    for(double x:final_table->scales())out.write(reinterpret_cast<const char*>(&x),sizeof(x));
    for(double x:final_table->fractions())out.write(reinterpret_cast<const char*>(&x),sizeof(x));
    for(auto row:final_table->values())for(double x:row)out.write(reinterpret_cast<const char*>(&x),sizeof(x));
    if(!out)throw std::runtime_error("table dump failed");
   } else if(op=="FSPLIT"||op=="FSUDA") {
    if(!final_table)throw std::runtime_error("FCONFIG required");
    std::string channel;double parent,q,zeta,energy,high=0;int angular;
    std::cin>>channel;if(op=="FSUDA")std::cin>>high;
    if(!(std::cin>>parent>>q>>zeta>>energy>>angular))throw std::runtime_error("bad final splitting query");
    jewel::Channel c;
    if(channel=="QQ")c=jewel::Channel::QQ;else if(channel=="QG")c=jewel::Channel::QG;
    else if(channel=="GG")c=jewel::Channel::GG;else if(channel=="GC")c=jewel::Channel::GC;else throw std::runtime_error("unsupported final channel");
    std::cout<<(op=="FSPLIT"?final_table->splitting(parent,q,zeta,energy,c,angular):final_table->sudakov(high,parent,q,zeta,energy,c,angular))<<'\n';
   } else if(op=="RCONFIG") {
    jewel::Kernels k;double q0,emax,scale;int boost;
    if(!(std::cin>>k.nf>>q0>>k.lambda_shower>>emax>>scale>>boost))throw std::runtime_error("invalid radiation configuration");
    radiation=std::make_unique<jewel::InitialRadiation>(k,q0);
    sudakov=std::make_unique<jewel::InitialSudakovTable>(k,q0,emax,scale,medium->maximum_scatterer(boost!=0));
   } else if(op=="RTABLE") {
    if(!sudakov)throw std::runtime_error("RCONFIG required");
    for(auto row:sudakov->table()) { for(double x:row)std::cout<<x<<' ';std::cout<<'\n'; }
   } else if(op=="RSPLIT"||op=="RSUDA"||op=="RRED"||op=="RFAST") {
    if(!sudakov)throw std::runtime_error("RCONFIG required");
    std::string channel;double x,y;if(!(std::cin>>channel>>x>>y))throw std::runtime_error("invalid radiation probe");
    jewel::Channel c;
    if(channel=="QQ")c=jewel::Channel::QQ;else if(channel=="QG")c=jewel::Channel::QG;
    else if(channel=="GG")c=jewel::Channel::GG;else if(channel=="GC")c=jewel::Channel::GC;
    else if(channel=="GQ")c=jewel::Channel::GQ;else throw std::runtime_error("unknown channel");
    double result=op=="RSPLIT"?radiation->splitting(x,c):op=="RSUDA"?radiation->sudakov(x,y,c):op=="RRED"?sudakov->reduced(x,c):sudakov->ratio(x,y,c);
    std::cout<<result<<'\n';
   } else if(op=="BOUNDS") {
    auto v=medium->bounds();std::cout<<v.temperature_max<<' '<<v.debye_max<<' '<<v.debye_min<<' '<<v.scatterer_mass_max<<' '<<v.density_at_debye_min<<' '<<v.lifetime_max<<' '<<v.density_max<<'\n';
   } else if(op=="AVERAGE"||op=="MAXIMUM") {
    jewel::SpaceTime v;int boost;
    if(op=="AVERAGE" && !(std::cin>>v.x>>v.y>>v.z>>v.t)) throw std::runtime_error("bad position");
    if(!(std::cin>>boost)) throw std::runtime_error("bad boost");
    auto a=op=="AVERAGE"?medium->average_scatterer(v,boost!=0):medium->maximum_scatterer(boost!=0);
    std::cout<<a.momentum.px<<' '<<a.momentum.py<<' '<<a.momentum.pz<<' '<<a.momentum.energy<<' '<<a.mass<<'\n';
   } else if(op=="EVENT"||op=="VERTEX"||op=="SCATTER") {
    double a=0,b=0; jewel::SpaceTime point; int boost=1;
    if(op=="SCATTER") std::cin>>point.x>>point.y>>point.z>>point.t>>boost;
    else std::cin>>a>>b;
    std::size_t count;
    if(!(std::cin>>count)||count>100000) throw std::runtime_error("invalid replay size");
    std::vector<double> tape(count); for(double& r:tape) if(!(std::cin>>r)) throw std::runtime_error("short replay");
    jewel::ReplayRandom random(tape);
    if(op=="EVENT") {
     jewel::GeometrySampler sampler(medium->cross_section_table());auto event=sampler.sample(a,b,random);
     *medium=medium->with_impact_parameter(event.impact_parameter);
     std::cout<<event.impact_parameter<<' '<<event.centrality_fraction;
    } else if(op=="VERTEX") {
     *medium=medium->with_impact_parameter(a);auto v=medium->sample_vertex(random,b);
     std::cout<<v.x<<' '<<v.y;
    } else {
     auto v=medium->sample_scatterer(point,random,boost!=0);
     std::cout<<v.type<<' '<<v.momentum.px<<' '<<v.momentum.py<<' '<<v.momentum.pz<<' '<<v.momentum.energy<<' '<<v.mass;
    }
    std::cout<<' '<<random.checkpoint()<<'\n';
   }
   else if(op=="CROSS") { for(auto row:medium->cross_section_table()) std::cout<<row[0]<<' '<<row[1]<<' '<<row[2]<<'\n'; }
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
