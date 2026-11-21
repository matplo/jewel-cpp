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
  require(copy.pdg==p.pdg&&(std::abs(copy.status)==62||std::abs(copy.status)==64),"unsupported hard-leg copy status/flavour");
  require(std::find(copy.mothers.begin(),copy.mothers.end(),id)!=copy.mothers.end(),"nonreciprocal hard-leg copy");
  id=next;
 }
}
std::string number(double value){std::ostringstream out;out<<std::setprecision(17)<<value;return out.str();}

}
struct Pythia8Input::Impl {
 Pythia8::Pythia engine;
 std::vector<std::string> settings;
 std::uint64_t attempts{},returned{};
 explicit Impl(const PythiaInputConfig& c):engine(c.xml_directory.empty()?JEWEL_PYTHIA8_XML:c.xml_directory,false){
  require(std::isfinite(c.ecm)&&std::isfinite(c.pt_min)&&std::isfinite(c.pt_max)&&c.ecm>0&&c.pt_min>0&&c.pt_max>c.pt_min&&2*c.pt_min<c.ecm,"invalid hard-process energy range");
  require(c.seed>0&&c.seed<=900000000,"Pythia seed must be between 1 and 900000000");
  require(c.pdf_set==13&&c.pp_tune==14,"base tune 14 and internal PDF 13 are required; use external_pdf for an LHAPDF override");
  require(c.external_pdf.empty()||std::regex_match(c.external_pdf,std::regex("[A-Za-z0-9_-]+/[0-9]+")),"external PDF requires set/member with no whitespace");
  require(c.hard_alpha_s.has_value()==c.hard_alpha_s_order.has_value(),"hard alpha_s value and order must be specified together");
  require(c.external_pdf.empty()||c.hard_alpha_s.has_value(),"external PDF requires explicit hard alpha_s and running order");
  if(c.hard_alpha_s)require(std::isfinite(*c.hard_alpha_s)&&*c.hard_alpha_s>=.06&&*c.hard_alpha_s<=.25&&*c.hard_alpha_s_order>=0&&*c.hard_alpha_s_order<=3,"invalid hard alpha_s policy");
  settings={"Tune:pp = 14","Beams:idA = 2212","Beams:idB = 2212","Beams:eCM = "+number(c.ecm),
   "HardQCD:all = on","PhaseSpace:pTHatMin = "+number(c.pt_min),"PhaseSpace:pTHatMax = "+number(c.pt_max),
   "PDF:pSet = "+(c.external_pdf.empty()?std::string("13"):"LHAPDF6:"+c.external_pdf),"HadronLevel:all = off","PartonLevel:FSR = off","PartonLevel:ISR = off","PartonLevel:MPI = off",
   "Check:event = on","Random:setSeed = on","Random:seed = "+std::to_string(c.seed),
   "Init:showChangedSettings = off","Init:showChangedParticleData = off","Init:showProcesses = off",
   "Next:numberShowInfo = 0","Next:numberShowProcess = 0","Next:numberShowEvent = 0"};
  if(c.hard_alpha_s){settings.push_back("SigmaProcess:alphaSvalue = "+number(*c.hard_alpha_s));settings.push_back("SigmaProcess:alphaSorder = "+std::to_string(*c.hard_alpha_s_order));}
  for(const auto& setting:settings)if(!engine.readString(setting))throw std::runtime_error("Pythia rejected setting: "+setting);
  if(!engine.init())throw std::runtime_error("Pythia input initialization failed");
  if(c.hard_alpha_s)require(engine.settings.parm("SigmaProcess:alphaSvalue")==*c.hard_alpha_s&&engine.settings.mode("SigmaProcess:alphaSorder")==*c.hard_alpha_s_order,"Pythia changed explicit hard coupling settings");
  if(!c.external_pdf.empty())require(engine.settings.word("PDF:pSet")=="LHAPDF6:"+c.external_pdf,"Pythia changed explicit external PDF");
 }
};
Pythia8Input::Pythia8Input(PythiaInputConfig config):impl_(std::make_unique<Impl>(config)){}
Pythia8Input::~Pythia8Input()=default;
int Pythia8Input::version(){return PYTHIA_VERSION_INTEGER;}
const std::vector<std::string>& Pythia8Input::settings() const{return impl_->settings;}
std::optional<HardEventSnapshot> Pythia8Input::next(){
 auto& p=impl_->engine;++impl_->attempts;
 if(!p.next())return std::nullopt;
 HardEventSnapshot result;result.attempt=impl_->attempts;
 const auto& info=p.info;result.process_code=info.code();result.weight=info.weight();result.pt_hat=info.pTHat();
 result.incoming_id1=info.id1();result.incoming_id2=info.id2();result.x1=info.x1();result.x2=info.x2();
 result.q_factorization=info.QFac();result.q_renormalization=info.QRen();
 std::vector<ParticleId> incoming,outgoing;
 result.particles=detail::capture_particles(p.event);
 for(const auto& node:result.particles){
  if(std::abs(node.status)==21)incoming.push_back(node.id);
  if(std::abs(node.status)==23)outgoing.push_back(node.id);
 }
 require(incoming.size()==2&&outgoing.size()==2,"unsupported hard-event topology: expected two incoming and two outgoing process roles");
 std::copy(incoming.begin(),incoming.end(),result.hard_incoming.begin());std::copy(outgoing.begin(),outgoing.end(),result.hard_outgoing.begin());
 for(std::size_t i=0;i<2;++i)result.hard_final[i]=final_copy(result,result.hard_outgoing[i]);
 for(int i=0;i<p.event.sizeJunction();++i)result.junctions.push_back({p.event.kindJunction(i),{p.event.colJunction(i,0),p.event.colJunction(i,1),p.event.colJunction(i,2)}});
 validate_hard_snapshot(result);++impl_->returned;return result;
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
  for(double v:p.vertex_mm)require(std::isfinite(v),"nonfinite input vertex");
  for(auto id:p.mothers){at(id);require(id!=p.id,"self-parent in snapshot");}
  for(auto id:p.daughters){at(id);require(id!=p.id,"self-daughter in snapshot");}
 }
 require(at(event.hard_incoming[0]).pdg==event.incoming_id1&&at(event.hard_incoming[1]).pdg==event.incoming_id2,"incoming flavour metadata mismatch");
 require(event.hard_final[0]!=event.hard_final[1],"duplicate final hard leg");
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
