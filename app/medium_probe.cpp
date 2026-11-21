// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/scattering.hpp"
#include "jewel/branching.hpp"
#include "jewel/shower.hpp"
#include "jewel/transfer.hpp"
#include "jewel/evolution.hpp"
#include "jewel/collision.hpp"
#include <iostream>
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
 jewel::EvolutionConfig evolution_config;
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
   if(op=="ECONFIG") {
    std::cin>>evolution_config.cutoff>>evolution_config.mass_scale>>evolution_config.formation_factor
     >>evolution_config.lifetime>>evolution_config.cross_section_scale;
   } else if(op=="PAIR") {
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
    std::size_t count;std::cin>>count;std::vector<double> tape(count);for(auto& a:tape)std::cin>>a;
    if(!std::cin)throw std::runtime_error("bad collision replay");jewel::ReplayRandom random(tape);
    jewel::CollisionPair result;
    try{result=jewel::scatter_pair(leg,scratch,transfer,total,remaining,target,delta,config,*branching,*medium,random);}
    catch(const std::domain_error&){
     std::cout<<"0 0 0 0 0 0 0 0 0 "<<random.checkpoint()<<" 1\n";
     for(int i=0;i<4;++i)std::cout<<"0 0 0 0 0 0 0 0 0 0 0\n";
     continue;
    }
    std::cout<<result.mass_change<<' '<<result.remaining_transfer<<' '<<result.target_mass<<' '<<result.recoil_fraction<<' '
     <<result.split_recoil<<' '<<result.recoil_pair<<' '<<result.rejected<<' '<<result.reshuffled<<' '<<result.colour_direction<<' '<<random.checkpoint()<<" 0\n";
    for(auto a:{result.centre,result.incoming,result.recoil,result.outgoing}){
     auto p=a.particle.momentum;auto v=a.generator_vertex;
     std::cout<<a.particle.status<<' '<<a.particle.pdg<<' '<<p.px<<' '<<p.py<<' '<<p.pz<<' '<<p.energy<<' '<<a.record_mass<<' '
      <<v.x<<' '<<v.y<<' '<<v.z<<' '<<v.t<<'\n';
    }
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
   } else if(op=="REJECT"||op=="SPLIT") {
    if(!branching)throw std::runtime_error("BCONFIG required");
    std::size_t n; jewel::ParticleId mother;std::cin>>n>>mother;
    if(n>23000||n==0)throw std::runtime_error("invalid event size");
    jewel::ShowerState state;double formation=1;
    if(op=="SPLIT")std::cin>>formation>>state.colour_max;
    std::vector<std::array<jewel::ParticleId,2>> links;
    for(std::size_t i=0;i<n;++i){
     jewel::ShowerParticle a;std::array<jewel::ParticleId,2> daughters;auto& p=a.particle.momentum;auto& v=a.generator_vertex;auto& x=a.particle.production;int qq,ao;
     std::cin>>a.particle.status>>a.particle.pdg>>a.parent>>daughters[0]>>daughters[1]
      >>p.px>>p.py>>p.pz>>p.energy>>a.record_mass>>v.x>>v.y>>v.z>>v.t
      >>x.x>>x.y>>x.z>>x.t>>a.decay_time>>a.evolution_time>>a.fraction>>a.angle>>qq>>ao;
     if(op=="SPLIT")std::cin>>a.particle.colour>>a.particle.anticolour;
     if(!std::cin)throw std::runtime_error("bad shower record");
     a.quark_pair=qq!=0;a.angular_required=ao!=0;state.append(a);links.push_back(daughters);
    }
    for(std::size_t i=0;i<n;++i)state.at(i+1).daughters=links[i];
    state.validate();std::size_t count;std::cin>>count;
    if(count>100000)throw std::runtime_error("bad replay size");
    std::vector<double> tape(count);for(auto& x:tape)if(!(std::cin>>x))throw std::runtime_error("short replay");
    jewel::ReplayRandom random(tape);
    if(op=="SPLIT")jewel::make_splitting(state,mother,*branching,*medium,formation,random);
    else jewel::reject_splitting(state,mother,*branching,random);
    state.validate();
    std::cout<<int(state.discarded)<<' '<<random.checkpoint();
    if(op=="SPLIT")std::cout<<' '<<state.particles().size()<<' '<<state.colour_max;
    std::cout<<'\n';
    for(const auto& a:state.particles()){
     auto p=a.particle.momentum;auto v=a.generator_vertex;auto x=a.particle.production;
     std::cout<<a.particle.status<<' '<<a.particle.pdg<<' '<<a.parent<<' '<<a.daughters[0]<<' '<<a.daughters[1]<<' '
      <<p.px<<' '<<p.py<<' '<<p.pz<<' '<<p.energy<<' '<<a.record_mass<<' '
      <<v.x<<' '<<v.y<<' '<<v.z<<' '<<v.t<<' '<<x.x<<' '<<x.y<<' '<<x.z<<' '<<x.t<<' '
      <<a.decay_time<<' '<<a.evolution_time<<' '<<a.fraction<<' '<<a.angle<<' '
      <<int(a.quark_pair)<<' '<<int(a.angular_required);
     if(op=="SPLIT")std::cout<<' '<<a.particle.colour<<' '<<a.particle.anticolour;
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
    else if(name=="GG")channel=jewel::Channel::GG;else if(name=="QG")channel=jewel::Channel::QG;else throw std::runtime_error("invalid branching channel");
    if(op=="MASS") {
     auto result=branching->mass(a,b,c,energy,channel,maxmass,initial!=0,random);
     std::cout<<result.mass<<' '<<result.fraction.value_or(-1)<<' '<<int(result.quark_pair.value_or(false))<<' '<<random.checkpoint()<<'\n';
    } else std::cout<<branching->fraction(a,b,c,channel,random)<<' '<<random.checkpoint()<<'\n';
   } else if(op=="XCONFIG") {
    jewel::Kernels k;double q0,emax,scale;int boost;
    if(!(std::cin>>k.nf>>q0>>k.lambda_shower>>k.lambda_scattering>>emax>>scale>>boost))throw std::runtime_error("bad rate configuration");
    rates=std::make_unique<jewel::ScatteringRates>(k,q0,emax,scale,*medium,boost!=0);
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
