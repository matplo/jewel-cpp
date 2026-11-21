// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/pythia8_hadronizer.hpp"
#include "pythia8_records.hpp"
#include <Pythia8/Pythia.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <unordered_set>
#include <iomanip>
#include <sstream>
namespace jewel {
std::vector<std::string> jewel260_fragmentation_settings(){
 auto setting=[](const char* name,double value){std::ostringstream out;out<<name<<" = "<<std::setprecision(17)<<value;return out.str();};
 // JEWEL assigns default-REAL literals into double-precision PARJ entries.
 const double light=double(.31f),strange=double(.4f),heavy=double(.54f);
 return {setting("StringFlav:probQQtoQ",double(.073f)),setting("StringFlav:probStoUD",double(.2f)),
  setting("StringFlav:probSQtoQQ",double(.94f)),setting("StringFlav:probQQ1toQQ0",double(.032f)),
  setting("StringFlav:mesonUDvector",light/(1-light)),setting("StringFlav:mesonSvector",strange/(1-strange)),
  setting("StringFlav:mesonCvector",heavy/(1-heavy)),setting("StringFlav:mesonBvector",heavy/(1-heavy)),
  setting("StringFlav:etaSup",double(.63f)),setting("StringFlav:etaPrimeSup",double(.12f)),
  setting("StringPT:sigma",double(.325f)),setting("StringZ:aLund",double(.5f)),setting("StringZ:bLund",double(.6f)),
  setting("StringZ:aExtraDiquark",.5),setting("StringZ:aExtraSQuark",0.),"StringZ:useOldAExtra = off",
  setting("StringZ:rFactC",1.),setting("StringZ:rFactB",double(.67f))};
}
struct Pythia8Hadronizer::Impl {
 Pythia8::Pythia engine;
 std::vector<std::string> settings;
 std::uint64_t attempts{},returned{};
 explicit Impl(const HadronizerConfig& c):engine(c.xml_directory.empty()?JEWEL_PYTHIA8_XML:c.xml_directory,false){
  if(c.seed<=0||c.seed>900000000)throw std::domain_error("invalid hadronization seed");
  settings={"Tune:pp = 14","ProcessLevel:all = off","PartonLevel:all = off","PartonLevel:FSR = off","PartonLevel:ISR = off","PartonLevel:MPI = off",
   "PartonLevel:FSRinResonances = off","ProcessLevel:resonanceDecays = off","HadronLevel:all = on",
   "HadronLevel:Decay = on","ColourReconnection:reconnect = off","ColourReconnection:forceHadronLevelCR = off","ParticleDecays:limitTau0 = on","ParticleDecays:tau0Max = 10",
   "Check:event = on","Random:setSeed = on","Random:seed = "+std::to_string(c.seed),
   "Init:showChangedSettings = off","Init:showChangedParticleData = off","Init:showProcesses = off",
   "Next:numberShowInfo = 0","Next:numberShowProcess = 0","Next:numberShowEvent = 0"};
  settings.push_back(std::string("111:mayDecay = ")+(c.pi0_decay?"on":"off"));
  if(c.fragmentation==FragmentationProfile::Jewel260Mapped){auto mapped=jewel260_fragmentation_settings();settings.insert(settings.end(),mapped.begin(),mapped.end());}
  else if(c.fragmentation!=FragmentationProfile::Monash14)throw std::domain_error("invalid fragmentation profile");
  for(const auto& setting:settings)if(!engine.readString(setting))throw std::runtime_error("Pythia rejected setting: "+setting);
  if(!engine.init())throw std::runtime_error("standalone hadronizer initialization failed");
 }
};
Pythia8Hadronizer::Pythia8Hadronizer(HadronizerConfig c):impl_(std::make_unique<Impl>(c)){}
Pythia8Hadronizer::~Pythia8Hadronizer()=default;
const std::vector<std::string>& Pythia8Hadronizer::settings() const{return impl_->settings;}
HadronizationStatistics Pythia8Hadronizer::statistics() const{return {impl_->attempts,impl_->returned,impl_->attempts-impl_->returned};}
std::optional<HadronizedSnapshot> Pythia8Hadronizer::hadronize(const std::vector<GeneratorParticle>& input,const std::vector<GeneratorJunction>& junctions){
 if(input.empty())throw std::domain_error("empty hadronization input");
 std::unordered_set<ParticleId> ids;Pythia8::Vec4 total;
 for(const auto& a:input){
  if(!a.id||!ids.insert(a.id).second||a.colour<0||a.anticolour<0)throw std::domain_error("invalid hadronization identity or colour");
  const auto& p=a.momentum;
  for(double v:{p.px,p.py,p.pz,p.energy,a.mass,a.scale})if(!std::isfinite(v))throw std::domain_error("nonfinite hadronization input");
  for(double v:a.vertex_mm)if(!std::isfinite(v))throw std::domain_error("nonfinite hadronization vertex");
  if(p.energy<=0||a.mass<0)throw std::domain_error("invalid hadronization energy/mass");
  if(std::abs(p.energy*p.energy-p.px*p.px-p.py*p.py-p.pz*p.pz-a.mass*a.mass)>1e-8*std::max(1.,p.energy*p.energy))throw std::domain_error("off-shell hadronization input");
  total+=Pythia8::Vec4(p.px,p.py,p.pz,p.energy);
 }
 auto& engine=impl_->engine;auto& event=engine.event;event.reset();HadronizedSnapshot result;
 for(const auto& a:input){const auto& p=a.momentum;
  if(!engine.particleData.isParticle(a.pdg))throw std::domain_error("unknown hadronization PDG ID");
  if(engine.particleData.isResonance(a.pdg))throw std::domain_error("resonance input requires a separately configured decay policy");
  auto i=event.append(a.pdg,23,a.colour,a.anticolour,p.px,p.py,p.pz,p.energy,a.mass,a.scale);
  event[i].vProd(a.vertex_mm[0],a.vertex_mm[1],a.vertex_mm[2],a.vertex_mm[3]);result.input_sources.push_back(a.id);
 }
 for(const auto& j:junctions){
  if(j.kind<1||j.kind>6||std::any_of(j.colours.begin(),j.colours.end(),[](int c){return c<=0;}))throw std::domain_error("invalid junction input");
  event.appendJunction(j.kind,j.colours[0],j.colours[1],j.colours[2]);
 }
 event[0].p(total);event[0].m(total.mCalc());++impl_->attempts;
 // Junctions are provided explicitly; do not infer a different topology.
 if(!engine.forceHadronLevel(false))return std::nullopt;
 result.particles=detail::capture_particles(event);
 Pythia8::Vec4 final;int initial_charge=0,final_charge=0;
 for(const auto& a:input)initial_charge+=engine.particleData.chargeType(a.pdg);
 for(const auto& a:result.particles){const auto& p=a.momentum;
  for(double v:{p.px,p.py,p.pz,p.energy})if(!std::isfinite(v))throw std::domain_error("nonfinite hadronization output");
  if(!a.final)continue;
  if(a.colour||a.anticolour)throw std::domain_error("unhadronized coloured final state");
  final+=Pythia8::Vec4(p.px,p.py,p.pz,p.energy);final_charge+=engine.particleData.chargeType(a.pdg);
 }
 if(initial_charge!=final_charge)throw std::domain_error("hadronization charge does not close");
 auto delta=final-total;
 for(double v:{delta.px(),delta.py(),delta.pz(),delta.e()})if(std::abs(v)>1e-8*std::max(1.,total.e()))throw std::domain_error("hadronization momentum does not close");
 ++impl_->returned;return result;
}
std::optional<HadronizedSnapshot> Pythia8Hadronizer::hadronize_string_pieces(
 const std::vector<GeneratorParticle>& input,const std::vector<LegacyMpiStringPiece>& pieces){
 std::vector<int> types;types.reserve(input.size());
 for(const auto& p:input){
  if(!p.final||!impl_->engine.particleData.isParticle(p.pdg))
   throw std::domain_error("MPI string hadronization requires known outgoing particles");
  types.push_back(impl_->engine.particleData.colType(p.pdg));
 }
 auto tags=legacy_mpi_colour_tags(pieces,types);auto particles=input;
 for(std::size_t i=0;i<particles.size();++i){particles[i].colour=tags[i][0];particles[i].anticolour=tags[i][1];}
 return hadronize(particles);
}
double Pythia8Hadronizer::nominal_mass(int pdg) const {
 auto& data=impl_->engine.particleData;
 if(!data.isParticle(pdg))throw std::domain_error("unknown projection PDG ID");
 double mass=data.m0(pdg);
 if(!std::isfinite(mass)||mass<0)throw std::domain_error("invalid nominal projection mass");
 return mass;
}

}
