// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/pythia8_input.hpp"
#include <Pythia8/Pythia.h>
#include "pythia8_records.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <regex>
#include <stdexcept>
#include <unordered_set>
namespace jewel {
namespace {
void require(bool ok,const char* message){if(!ok)throw std::domain_error(message);}
ParticleId final_copy(const HardEventSnapshot& event,ParticleId root){
 std::unordered_set<ParticleId> visited;auto id=root;
 for(;;){
  require(id>0&&id<=event.particles.size()&&visited.insert(id).second,"invalid hard-leg copy chain");
  const auto& p=event.particles[id-1];if(p.final)return id;
  require(p.daughters.size()==1,"unsupported branching hard-leg history");
  auto next=p.daughters.front();require(next>0&&next<=event.particles.size(),"missing hard-leg copy");
  const auto& copy=event.particles[next-1];
  require(copy.pdg==p.pdg&&(std::abs(copy.status)==62||std::abs(copy.status)==64||(event.initial_state_radiation&&std::abs(copy.status)==44)),"unsupported hard-leg copy status/flavour");
  require(std::find(copy.mothers.begin(),copy.mothers.end(),id)!=copy.mothers.end(),"nonreciprocal hard-leg copy");
  id=next;
 }
}
std::string number(double value){std::ostringstream out;out<<std::setprecision(17)<<value;return out.str();}

}
// Observe parton-level output immediately before BeamSetup::boostAndVertex.
// This hook never vetoes, changes records, or consumes random numbers.
class EnergyStageCapture final : public Pythia8::UserHooks {
 struct Candidate {GeneratorIsrFractions fractions;Momentum sister;int pdg{};};
 std::optional<Candidate> pending;
 std::vector<Candidate> completed;
 static bool matches(const Candidate& c,const Pythia8::Event& event){
  auto i=static_cast<int>(c.fractions.emitted)-1;
  return i>=0&&i<event.size()&&std::abs(event[i].status())==43&&event[i].id()==c.pdg
   &&event[i].px()==c.sister.px&&event[i].py()==c.sister.py&&event[i].pz()==c.sister.pz;
 }
 void finish_pending(const Pythia8::Event& event){
  if(pending){const auto system=pending->fractions.subsystem;
   if(matches(*pending,event)&&system<beamAPtr->size()&&system<beamBPtr->size()){
    pending->fractions.after={(*beamAPtr)[system].x(),(*beamBPtr)[system].x()};completed.push_back(*pending);
   }pending.reset();
  }
 }
public:
 std::vector<Momentum> momenta;
 std::vector<GeneratorIsrFractions> fractions;
 void reset(){momenta.clear();fractions.clear();pending.reset();completed.clear();}
 bool canVetoISREmission() override{return true;}
 bool doVetoISREmission(int sizeOld,const Pythia8::Event& event,int system) override{
  // Before the next tentative branch commits, beam slots still contain the
  // preceding accepted branch's updated fractions. Discard rolled-back roots.
  finish_pending(event);
  Candidate c;c.fractions.subsystem=system;c.fractions.collision_s=infoPtr->s();
  require(system>=0&&system<beamAPtr->size()&&system<beamBPtr->size(),"missing ISR beam slots");
  c.fractions.before={(*beamAPtr)[system].x(),(*beamBPtr)[system].x()};
  for(int i=sizeOld;i<event.size();++i){int status=std::abs(event[i].status());
   if(status==41){require(!c.fractions.mother,"multiple observed ISR mothers");c.fractions.mother=i+1;}
   if(status==42){require(!c.fractions.recoiler,"multiple observed ISR recoilers");c.fractions.recoiler=i+1;}
   if(status==43){require(!c.fractions.emitted,"multiple observed ISR emissions");c.fractions.emitted=i+1;c.pdg=event[i].id();c.sister={event[i].px(),event[i].py(),event[i].pz(),event[i].e()};}
  }
  require(c.fractions.mother&&c.fractions.recoiler&&c.fractions.emitted,"unsupported ISR observation topology");pending=c;return false;
 }
 bool canVetoPartonLevel() override{return true;}
 bool doVetoPartonLevel(const Pythia8::Event& event) override{
  finish_pending(event);fractions.clear();
  for(const auto& c:completed)if(matches(c,event))fractions.push_back(c.fractions);
  momenta.clear();for(int i=0;i<event.size();++i){auto p=event[i].p();momenta.push_back({p.px(),p.py(),p.pz(),p.e()});}return false;
 }
};
struct Pythia8Input::Impl {
 std::shared_ptr<EnergyStageCapture> capture=std::make_shared<EnergyStageCapture>();
 Pythia8::Pythia engine;
 std::vector<std::string> settings;
 std::uint64_t attempts{},returned{};
 bool initial_state_radiation{},multiple_parton_interactions{};
 explicit Impl(const PythiaInputConfig& c):engine(c.xml_directory.empty()?JEWEL_PYTHIA8_XML:c.xml_directory,false),initial_state_radiation(c.initial_state_radiation),multiple_parton_interactions(c.multiple_parton_interactions){
  require(std::isfinite(c.ecm)&&std::isfinite(c.pt_min)&&std::isfinite(c.pt_max)&&c.ecm>0&&c.pt_min>0&&c.pt_max>c.pt_min&&2*c.pt_min<c.ecm,"invalid hard-process energy range");
  require(c.seed>0&&c.seed<=900000000,"Pythia seed must be between 1 and 900000000");
  require(c.pdf_set==13&&c.pp_tune==14,"base tune 14 and internal PDF 13 are required; use external_pdf for an LHAPDF override");
  require(c.external_pdf.empty()||std::regex_match(c.external_pdf,std::regex("[A-Za-z0-9_-]+/[0-9]+")),"external PDF requires set/member with no whitespace");
  require(c.hard_alpha_s.has_value()==c.hard_alpha_s_order.has_value(),"hard alpha_s value and order must be specified together");
  require(c.external_pdf.empty()||c.hard_alpha_s.has_value(),"external PDF requires explicit hard alpha_s and running order");
  if(c.hard_alpha_s)require(std::isfinite(*c.hard_alpha_s)&&*c.hard_alpha_s>=.06&&*c.hard_alpha_s<=.25&&*c.hard_alpha_s_order>=0&&*c.hard_alpha_s_order<=3,"invalid hard alpha_s policy");
  require(c.isr_alpha_s.has_value()==c.isr_alpha_s_order.has_value(),"ISR alpha_s value and order must be specified together");
  if(c.isr_alpha_s)require(std::isfinite(*c.isr_alpha_s)&&*c.isr_alpha_s>=.06&&*c.isr_alpha_s<=.25&&*c.isr_alpha_s_order>=0&&*c.isr_alpha_s_order<=3,"invalid ISR alpha_s policy");
  if(c.alpha_s_nfmax)require(*c.alpha_s_nfmax==5||*c.alpha_s_nfmax==6,"alpha_s maximum flavours must be 5 or 6");
  require(c.mpi_alpha_s.has_value()==c.mpi_alpha_s_order.has_value(),"MPI alpha_s value and order must be specified together");
  if(c.mpi_alpha_s)require(std::isfinite(*c.mpi_alpha_s)&&*c.mpi_alpha_s>=.06&&*c.mpi_alpha_s<=.25&&*c.mpi_alpha_s_order>=0&&*c.mpi_alpha_s_order<=3,"invalid MPI alpha_s policy");
  if(c.mpi_screening){const auto& v=*c.mpi_screening;
   require(std::isfinite(v[0])&&std::isfinite(v[1])&&std::isfinite(v[2])&&v[0]>=.5&&v[0]<=10&&v[1]>=1&&v[2]>=0&&v[2]<=.5,"invalid MPI screening policy");}
  if(c.mpi_double_gaussian){const auto& v=*c.mpi_double_gaussian;
   require(std::isfinite(v[0])&&std::isfinite(v[1])&&v[0]>=0&&v[0]<=1&&v[1]>=.1&&v[1]<=1,"invalid MPI double-Gaussian profile");}
  settings={"Tune:pp = 14","Beams:idA = 2212","Beams:idB = 2212","Beams:eCM = "+number(c.ecm),
   "HardQCD:all = on","PhaseSpace:pTHatMin = "+number(c.pt_min),"PhaseSpace:pTHatMax = "+number(c.pt_max),
   "PDF:pSet = "+(c.external_pdf.empty()?std::string("13"):"LHAPDF6:"+c.external_pdf),"HadronLevel:all = off","PartonLevel:FSR = off",std::string("PartonLevel:ISR = ")+(c.initial_state_radiation?"on":"off"),std::string("PartonLevel:MPI = ")+(c.multiple_parton_interactions?"on":"off"),
   "Check:event = on","Random:setSeed = on","Random:seed = "+std::to_string(c.seed),
   "Init:showChangedSettings = off","Init:showChangedParticleData = off","Init:showProcesses = off",
   "Next:numberShowInfo = 0","Next:numberShowProcess = 0","Next:numberShowEvent = 0"};
  if(c.pthat_bias_power){
   require(std::isfinite(*c.pthat_bias_power)&&*c.pthat_bias_power>=0&&*c.pthat_bias_power<=10,
    "pThat bias power must be in [0,10]");
   settings.push_back("PhaseSpace:bias2Selection = on");
   settings.push_back("PhaseSpace:bias2SelectionPow = "+number(*c.pthat_bias_power));
   settings.push_back("PhaseSpace:bias2SelectionRef = 1");
  }
  if(c.hard_alpha_s){settings.push_back("SigmaProcess:alphaSvalue = "+number(*c.hard_alpha_s));settings.push_back("SigmaProcess:alphaSorder = "+std::to_string(*c.hard_alpha_s_order));}
  if(c.isr_alpha_s){settings.push_back("SpaceShower:alphaSvalue = "+number(*c.isr_alpha_s));settings.push_back("SpaceShower:alphaSorder = "+std::to_string(*c.isr_alpha_s_order));}
  if(c.alpha_s_nfmax)settings.push_back("StandardModel:alphaSnfmax = "+std::to_string(*c.alpha_s_nfmax));
  if(c.isr_alpha_s_cmw)settings.push_back(std::string("SpaceShower:alphaSuseCMW = ")+(*c.isr_alpha_s_cmw?"on":"off"));
  if(c.mpi_alpha_s){settings.push_back("MultipartonInteractions:alphaSvalue = "+number(*c.mpi_alpha_s));settings.push_back("MultipartonInteractions:alphaSorder = "+std::to_string(*c.mpi_alpha_s_order));}
  if(c.mpi_screening){const auto& v=*c.mpi_screening;
   settings.push_back("MultipartonInteractions:pT0parametrization = 0");
   settings.push_back("MultipartonInteractions:pT0Ref = "+number(v[0]));
   settings.push_back("MultipartonInteractions:ecmRef = "+number(v[1]));
   settings.push_back("MultipartonInteractions:ecmPow = "+number(v[2]));}
  if(c.mpi_double_gaussian){const auto& v=*c.mpi_double_gaussian;
   settings.push_back("MultipartonInteractions:bProfile = 2");
   settings.push_back("MultipartonInteractions:coreFraction = "+number(v[0]));
   settings.push_back("MultipartonInteractions:coreRadius = "+number(v[1]));}
  for(const auto& setting:settings)if(!engine.readString(setting))throw std::runtime_error("Pythia rejected setting: "+setting);
  require(engine.setUserHooksPtr(capture),"cannot register energy-stage observer");
  if(!engine.init())throw std::runtime_error("Pythia input initialization failed");
  if(c.hard_alpha_s)require(engine.settings.parm("SigmaProcess:alphaSvalue")==*c.hard_alpha_s&&engine.settings.mode("SigmaProcess:alphaSorder")==*c.hard_alpha_s_order,"Pythia changed explicit hard coupling settings");
  if(c.isr_alpha_s)require(engine.settings.parm("SpaceShower:alphaSvalue")==*c.isr_alpha_s&&engine.settings.mode("SpaceShower:alphaSorder")==*c.isr_alpha_s_order,"Pythia changed explicit ISR coupling settings");
  if(c.alpha_s_nfmax)require(engine.settings.mode("StandardModel:alphaSnfmax")==*c.alpha_s_nfmax,"Pythia changed explicit flavour cap");
  if(c.isr_alpha_s_cmw)require(engine.settings.flag("SpaceShower:alphaSuseCMW")==*c.isr_alpha_s_cmw,"Pythia changed explicit ISR CMW setting");
  if(c.mpi_alpha_s)require(engine.settings.parm("MultipartonInteractions:alphaSvalue")==*c.mpi_alpha_s&&engine.settings.mode("MultipartonInteractions:alphaSorder")==*c.mpi_alpha_s_order,"Pythia changed explicit MPI coupling settings");
  if(c.mpi_screening){const auto& v=*c.mpi_screening;
   require(engine.settings.mode("MultipartonInteractions:pT0parametrization")==0&&engine.settings.parm("MultipartonInteractions:pT0Ref")==v[0]&&engine.settings.parm("MultipartonInteractions:ecmRef")==v[1]&&engine.settings.parm("MultipartonInteractions:ecmPow")==v[2],"Pythia changed explicit MPI screening settings");}
  if(c.mpi_double_gaussian){const auto& v=*c.mpi_double_gaussian;
   require(engine.settings.mode("MultipartonInteractions:bProfile")==2&&engine.settings.parm("MultipartonInteractions:coreFraction")==v[0]&&engine.settings.parm("MultipartonInteractions:coreRadius")==v[1],"Pythia changed explicit MPI overlap settings");}
  if(!c.external_pdf.empty())require(engine.settings.word("PDF:pSet")=="LHAPDF6:"+c.external_pdf,"Pythia changed explicit external PDF");
 }
};
Pythia8Input::Pythia8Input(PythiaInputConfig config):impl_(std::make_unique<Impl>(config)){}
Pythia8Input::~Pythia8Input()=default;
double Pythia8Input::nominal_mass(int pdg) const{
 auto& data=impl_->engine.particleData;require(data.isParticle(pdg),"unknown input nominal-mass PDG");
 double mass=data.m0(pdg);require(std::isfinite(mass)&&mass>=0,"invalid input nominal mass");return mass;
}
int Pythia8Input::version(){return PYTHIA_VERSION_INTEGER;}
const std::vector<std::string>& Pythia8Input::settings() const{return impl_->settings;}
std::optional<HardEventSnapshot> Pythia8Input::next(){
 auto& p=impl_->engine;++impl_->attempts;
 impl_->capture->reset();
 if(!p.next())return std::nullopt;
 HardEventSnapshot result;result.isr_fractions=impl_->capture->fractions;result.attempt=impl_->attempts;result.initial_state_radiation=impl_->initial_state_radiation;result.multiple_parton_interactions=impl_->multiple_parton_interactions;
 const auto& info=p.info;result.process_code=info.code();result.weight=info.weight();result.pt_hat=info.pTHat();
 result.incoming_id1=info.id1();result.incoming_id2=info.id2();result.x1=info.x1();result.x2=info.x2();
 result.q_factorization=info.QFac();result.q_renormalization=info.QRen();
 std::vector<ParticleId> incoming,outgoing;
 result.particles=detail::capture_particles(p.event);
 require(impl_->capture->momenta.size()==result.particles.size(),"missing pre-repair parton record");
 for(std::size_t i=0;i<result.particles.size();++i){auto& node=result.particles[i];const auto& before=impl_->capture->momenta[i];
  require(before.px==node.momentum.px&&before.py==node.momentum.py&&before.pz==node.momentum.pz,"unsupported post-parton spatial transformation");
  node.energy_before_beam_repair=before.energy;
 }
 for(const auto& node:result.particles){
  if(std::abs(node.status)==21)incoming.push_back(node.id);
  if(std::abs(node.status)==23)outgoing.push_back(node.id);
  if(std::abs(node.status)==43)result.isr_emissions.push_back(node.id);
 }
 require(incoming.size()==2&&outgoing.size()==2,"unsupported hard-event topology: expected two incoming and two outgoing process roles");
 std::copy(incoming.begin(),incoming.end(),result.hard_incoming.begin());std::copy(outgoing.begin(),outgoing.end(),result.hard_outgoing.begin());
 for(std::size_t i=0;i<2;++i)result.hard_final[i]=final_copy(result,result.hard_outgoing[i]);
 for(auto id:result.isr_emissions)result.isr_final.push_back(final_copy(result,id));
 // Cross-check the explicit, reciprocal copy walk against Pythia's own record navigation.
 for(std::size_t i=0;i<2;++i)require(result.hard_final[i]==static_cast<ParticleId>(p.event[result.hard_outgoing[i]-1].iBotCopy()+1),"Pythia hard-copy navigation disagrees");
 for(std::size_t i=0;i<result.isr_emissions.size();++i)require(result.isr_final[i]==static_cast<ParticleId>(p.event[result.isr_emissions[i]-1].iBotCopy()+1),"Pythia ISR-copy navigation disagrees");
 // Capture Pythia's subsystem identity rather than inferring pairs from pT.
 for(int i=0;i<p.partonSystems.sizeSys();++i){GeneratorSubsystem system;system.index=i;
  require(p.partonSystems.hasInAB(i)&&!p.partonSystems.hasInRes(i),"unsupported non-beam subsystem");
  system.incoming={static_cast<ParticleId>(p.partonSystems.getInA(i)+1),static_cast<ParticleId>(p.partonSystems.getInB(i)+1)};
  system.shat=p.partonSystems.getSHat(i);system.pt_hat=p.partonSystems.getPTHat(i);
  require(i<p.beamA.size()&&i<p.beamB.size(),"missing subsystem beam slots");
  require(p.beamA[i].isFromBeam()&&p.beamB[i].isFromBeam(),"rescattered subsystem beam capture unsupported");
  require(p.beamA[i].iPos()+1==static_cast<int>(system.incoming[0])&&p.beamB[i].iPos()+1==static_cast<int>(system.incoming[1]),"subsystem beam slot identity mismatch");
  auto target=[](const Pythia8::ResolvedParton& beam){auto v=beam.p();return Momentum{v.px(),v.py(),v.pz(),v.e()};};
  system.primordial_targets=std::array<Momentum,2>{target(p.beamA[i]),target(p.beamB[i])};
  for(int j=0;j<p.partonSystems.sizeOut(i);++j){auto id=static_cast<ParticleId>(p.partonSystems.getOut(i,j)+1);system.outgoing.push_back(id);system.outgoing_final.push_back(final_copy(result,id));}
  for(const auto& node:result.particles)if(std::abs(node.status)==23||std::abs(node.status)==33){
   auto final=final_copy(result,node.id);
   if(std::find(system.outgoing_final.begin(),system.outgoing_final.end(),final)!=system.outgoing_final.end()){
    require(final==static_cast<ParticleId>(p.event[node.id-1].iBotCopy()+1),"Pythia subsystem hard-copy navigation disagrees");
    system.process_outgoing.push_back(node.id);
   }
  }
  result.subsystems.push_back(std::move(system));
 }
 for(int i=0;i<p.event.sizeJunction();++i)result.junctions.push_back({p.event.kindJunction(i),{p.event.colJunction(i,0),p.event.colJunction(i,1),p.event.colJunction(i,2)}});
 validate_hard_snapshot(result);++impl_->returned;return result;
}
std::vector<std::array<ParticleId,2>> subsystem_hard_final_pairs(const HardEventSnapshot& input){
 validate_hard_snapshot(input);std::vector<std::array<ParticleId,2>> result;
 for(const auto& system:input.subsystems)result.push_back({final_copy(input,system.process_outgoing[0]),final_copy(input,system.process_outgoing[1])});
 require(!result.empty()&&result.front()==input.hard_final,"primary subsystem hard-copy mismatch");
 return result;
}
GeneratorStatistics Pythia8Input::statistics() const {
 const auto& i=impl_->engine.info;
 return {impl_->attempts,impl_->returned,impl_->attempts-impl_->returned,i.nAccepted(),i.sigmaGen(),i.sigmaErr(),i.weightSum()};
}
void validate_hard_snapshot(const HardEventSnapshot& event){
 require(!event.particles.empty(),"empty hard-event snapshot");
 for(double x:{event.weight,event.pt_hat,event.x1,event.x2,event.q_factorization,event.q_renormalization})require(std::isfinite(x),"nonfinite hard-event metadata");
 require(event.pt_hat>0&&event.q_factorization>0&&event.q_renormalization>0,"invalid hard-process scale");
 require(event.x1>0&&event.x1<=1&&event.x2>0&&event.x2<=1,"invalid incoming momentum fraction");
 auto at=[&](ParticleId id)->const GeneratorParticle&{require(id>0&&id<=event.particles.size(),"missing snapshot particle");return event.particles[id-1];};
 for(std::size_t i=0;i<event.particles.size();++i){const auto& p=event.particles[i];
  require(p.id==i+1&&p.source_index==static_cast<int>(i),"snapshot identity mapping changed");
  require(p.colour>=0&&p.anticolour>=0,"negative input colour tag");
  for(double v:{p.momentum.px,p.momentum.py,p.momentum.pz,p.momentum.energy,p.mass,p.scale})require(std::isfinite(v),"nonfinite input kinematics");
  if(p.energy_before_beam_repair)require(std::isfinite(*p.energy_before_beam_repair),"nonfinite pre-repair energy");
  for(double v:p.vertex_mm)require(std::isfinite(v),"nonfinite input vertex");
  for(auto id:p.mothers){at(id);require(id!=p.id,"self-parent in snapshot");}
  for(auto id:p.daughters){at(id);require(id!=p.id,"self-daughter in snapshot");}
 }
 require(at(event.hard_incoming[0]).pdg==event.incoming_id1&&at(event.hard_incoming[1]).pdg==event.incoming_id2,"incoming flavour metadata mismatch");
 require(!event.subsystems.empty(),"missing Pythia subsystem ledger");
 require(event.multiple_parton_interactions||event.subsystems.size()==1,"multiple systems in MPI-off snapshot");
 std::unordered_set<ParticleId> subsystem_outputs,process_roots;
 for(std::size_t i=0;i<event.subsystems.size();++i){const auto& system=event.subsystems[i];
  require(system.index==static_cast<int>(i)&&std::isfinite(system.shat)&&system.shat>0&&std::isfinite(system.pt_hat)&&system.pt_hat>0,"invalid subsystem metadata");
  if(system.primordial_targets)for(const auto& p:*system.primordial_targets)for(double v:{p.px,p.py,p.pz,p.energy})require(std::isfinite(v),"nonfinite subsystem target momentum");
  require(system.incoming[0]!=system.incoming[1],"duplicate subsystem incoming line");for(auto id:system.incoming)at(id);
  require(system.outgoing.size()==system.outgoing_final.size(),"incomplete subsystem final associations");
  for(std::size_t j=0;j<system.outgoing.size();++j){auto final=final_copy(event,system.outgoing[j]);
   require(final==system.outgoing_final[j]&&subsystem_outputs.insert(final).second,"duplicate or invalid subsystem final membership");}
  require(system.process_outgoing.size()==2,"subsystem is not a mapped two-body scattering");
  const auto& first=at(system.process_outgoing[0]);const auto& second=at(system.process_outgoing[1]);
  require(first.mothers.size()==2&&first.mothers==second.mothers,"scattering pair has inconsistent incoming ancestry");
  require(first.mothers[0]!=first.mothers[1],"duplicate scattering incoming root");
  std::array<double,4> pair_balance{};double pair_energy=0;
  for(int side=0;side<2;++side)for(auto id:side?system.process_outgoing:first.mothers){const auto& node=at(id);
   if(!side)require(std::abs(node.status)==(i==0?21:31),"incorrect scattering incoming root status");
   double sign=side?-1:1;const auto& v=node.momentum;
   pair_balance[0]+=sign*v.px;pair_balance[1]+=sign*v.py;pair_balance[2]+=sign*v.pz;pair_balance[3]+=sign*v.energy;
   pair_energy+=std::abs(v.energy);
  }
  for(double v:pair_balance)require(std::abs(v)<=1e-8*std::max(1.,pair_energy),"scattering root four-momentum does not close");
  for(auto id:system.process_outgoing){const auto& node=at(id);
   require(std::abs(node.status)==(i==0?23:33)&&process_roots.insert(id).second,"incorrect subsystem process role");
   auto final=final_copy(event,id);require(std::find(system.outgoing_final.begin(),system.outgoing_final.end(),final)!=system.outgoing_final.end(),"process root outside its subsystem");}
 }
 for(const auto& node:event.particles)if(std::abs(node.status)==23||std::abs(node.status)==33)require(process_roots.count(node.id)==1,"unmapped scattering root");
 std::unordered_set<ParticleId> expected_outputs;
 for(const auto& node:event.particles)if(std::abs(node.status)==23||std::abs(node.status)==33||std::abs(node.status)==43)
  require(expected_outputs.insert(final_copy(event,node.id)).second,"duplicate scattering/emission final copy");
 require(expected_outputs==subsystem_outputs,"subsystem membership omits radiation or includes unrelated particles");
 require(event.hard_final[0]!=event.hard_final[1],"duplicate final hard leg");
 require(event.isr_emissions.size()==event.isr_final.size(),"incomplete ISR final-copy associations");
 std::unordered_set<ParticleId> fraction_emissions;
 for(const auto& f:event.isr_fractions){
  require(std::isfinite(f.collision_s)&&f.collision_s>0,"invalid ISR collision energy squared");
  require(f.subsystem>=0&&static_cast<std::size_t>(f.subsystem)<event.subsystems.size(),"invalid ISR fraction subsystem");
  require(fraction_emissions.insert(f.emitted).second,"duplicate ISR fraction observation");
  require(std::abs(at(f.emitted).status)==43&&std::abs(at(f.mother).status)==41&&std::abs(at(f.recoiler).status)==42,"invalid ISR fraction roles");
  require(at(f.emitted).mothers==std::vector<ParticleId>{f.mother},"ISR fraction ancestry mismatch");
  for(auto values:{f.before,f.after})for(double x:values)require(std::isfinite(x)&&x>0&&x<=1,"invalid ISR beam fraction");
  require((f.after[0]>f.before[0]&&f.after[1]==f.before[1])||(f.after[1]>f.before[1]&&f.after[0]==f.before[0]),"ISR observation must advance exactly one beam fraction");
  auto final=final_copy(event,f.emitted);const auto& system=event.subsystems[f.subsystem];
  require(std::find(system.outgoing_final.begin(),system.outgoing_final.end(),final)!=system.outgoing_final.end(),"ISR fraction subsystem mismatch");
 }
 require(fraction_emissions==std::unordered_set<ParticleId>(event.isr_emissions.begin(),event.isr_emissions.end()),"missing ISR fraction observations");
 require(event.initial_state_radiation||event.isr_emissions.empty(),"ISR emissions in ISR-off snapshot");
 std::vector<ParticleId> emissions;
 for(const auto& p:event.particles)if(std::abs(p.status)==43)emissions.push_back(p.id);
 require(emissions==event.isr_emissions,"missing or duplicated ISR emission roots");
 std::unordered_set<ParticleId> final_roles(event.hard_final.begin(),event.hard_final.end());
 for(std::size_t i=0;i<event.isr_emissions.size();++i){
  require(event.isr_final[i]==final_copy(event,event.isr_emissions[i]),"incorrect ISR final-copy association");
  require(final_roles.insert(event.isr_final[i]).second,"duplicate final ISR/hard role");
 }
 for(std::size_t i=0;i<2;++i)require(event.hard_final[i]==final_copy(event,event.hard_outgoing[i]),"incorrect final hard-leg association");
 std::array<double,4> balance{};double energy=0;
 std::unordered_set<ParticleId> roles;
 for(int side=0;side<2;++side)for(auto id:side?event.hard_outgoing:event.hard_incoming){const auto& p=at(id);
  require(roles.insert(id).second,"duplicate hard-process role");require(std::abs(p.status)==(side?23:21),"incorrect hard-process role status");
  if(side)require(p.pdg==21||(std::abs(p.pdg)>=1&&std::abs(p.pdg)<=6),"nonpartonic hard-dijet output");
  const auto& v=p.momentum;double sign=side?-1:1;
  balance[0]+=sign*v.px;balance[1]+=sign*v.py;balance[2]+=sign*v.pz;balance[3]+=sign*v.energy;energy+=std::abs(v.energy);
 }
 for(double value:balance)require(std::abs(value)<=1e-8*std::max(1.,energy),"hard-process four-momentum does not close");
}
}
