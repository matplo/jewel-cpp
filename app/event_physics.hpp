// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/cascade.hpp"
#include "jewel/pythia8_shower_input.hpp"
#include "jewel/pythia8_shower_output.hpp"
#include <cmath>
#include <stdexcept>

namespace jewel {

// The CLI and the in-process generator share the same initialization,
// transport tables, cascade, and string construction.
class EventPhysics {
 Kernels kernels_;
 bool boost_;
 InitialSudakovTable sudakov_;
 FinalSplittingTable splitting_;
 PartonicPdf pdf_;
 ScatteringRates rates_;
 BranchingSampler branching_;
public:
 EventPhysics(const MediumModel& medium,double table_energy,bool boost,int angular_ordering)
  : boost_(boost),
    sudakov_(kernels_,1,table_energy,1,medium.maximum_scatterer(boost)),
    splitting_(kernels_,1,table_energy,1,medium.maximum_scatterer(boost)),
    pdf_(kernels_,1,table_energy,1,medium.maximum_scatterer(boost)),
    rates_(kernels_,1,table_energy,1,medium,boost),branching_(kernels_,1,1,angular_ordering) {}

 PreparedDijet prepare(const HardEventSnapshot& hard,const Pythia8Input& input,
  SpaceTime vertex,double lifetime,double max_rapidity,bool imported_input, Random& random,
  std::string& stage,const std::function<void(ImportedPythiaRecord&)>& before_preparation={}) const {
  stage="hard initialization";
  HardInitializationConfig config;
  config.x=vertex.x;config.y=vertex.y;config.lifetime=lifetime;
  if(!imported_input)return prepare_pythia_dijet(hard,config,branching_,random);
  stage="ISR mapping";
  auto chains=prepare_subsystem_isr_timing(hard,[&](int pdg){return input.nominal_mass(pdg);});
  auto imported=import_pythia_final_record(hard,vertex.x,vertex.y,lifetime);
  stage="ISR timing";
  IsrTimeConfig timing;
  timing.x=vertex.x;timing.y=vertex.y;timing.lifetime=lifetime;
  attach_pythia_isr_times(imported.state,imported.input_sources,hard,chains,timing,random);
  if(before_preparation)before_preparation(imported);
  stage="survivor preparation";
  prepare_imported_partons(imported,[&](int pdg){return input.nominal_mass(pdg);},max_rapidity);
  stage="subsystem initialization";
  auto initialized=initialize_imported_subsystems(imported,config,branching_,random,
                                                 MpiChannelPolicy::outgoing_flavour);
  PreparedDijet prepared;
  prepared.hard=imported.subsystems.front().hard;
  prepared.hard_process=imported.subsystems.front().hard_origins[0];
  prepared.scratch=imported.scratch;prepared.weight=imported.state.weight;
  prepared.initialization=initialized.pairs.front();
  prepared.input_sources=std::move(imported.input_sources);
  prepared.state=std::move(imported.state);
  return prepared;
 }

 CascadeResult evolve(PreparedDijet& event,const MediumModel& medium,
  InitialEvolutionConfig config,double lifetime,std::optional<double> impact_parameter,
  RecoilStorage& ledger,Random& random) const {
  std::optional<ScatteringRates> rebound;
  if(impact_parameter)rebound.emplace(rates_.with_event_impact_parameter(*impact_parameter));
  const auto& rates=rebound?*rebound:rates_;
  TransferSampler transfers(kernels_,medium,boost_);
  config.evolution.lifetime=lifetime;config.coherent.lifetime=lifetime;
  InitialScatteringEvolution initial(config,kernels_,branching_,sudakov_,splitting_,pdf_,rates,transfers,medium);
  ScatteringEvolution final(config.evolution,branching_,sudakov_,splitting_,rates,transfers);
  Cascade cascade({config.evolution,config.coherent.collision,config.coherent.finalization,1000000,true},
                  initial,final,branching_,medium);
  TransferHistory history;
  history.records.resize(100);
  const auto result=cascade.evolve(event.state,event.scratch,history,ledger,random);
  event.state.validate();
  if(event.state.discarded)throw std::runtime_error("cascade discarded event");
  for(const auto& node:event.state.particles()){
   const auto& particle=node.particle;
   if(particle.status==1||particle.status==2)throw std::runtime_error("unfinished cascade record");
   if(!config.coherent.finalization.hadronize_recoils&&particle.status==3)
    event.state.at(particle.id).particle.status=22;
  }
  return result;
 }

 std::vector<GeneratorParticle> strings(const ShowerState& state,const HardEventSnapshot& hard,
  bool imported,bool keep_recoils,int hadrotype,double max_rapidity,Random& random) const {
  if(imported)return prepare_native_hadronization(state,hard.junctions,keep_recoils);
  if(hadrotype==0)return prepare_vacuum_hadronization(state,keep_recoils);
  return prepare_minimum_hadronization(state,branching_,random,max_rapidity,keep_recoils);
 }
};

inline double jewel_table_energy(const PythiaInputConfig& input,double max_rapidity){
 return std::min(1.5*(input.pt_max+50.)*std::cosh(max_rapidity),input.ecm/2.);
}

inline std::uint64_t jewel_shower_seed(unsigned long seed,std::uint64_t attempt){
 return static_cast<std::uint64_t>(seed)+1000000+100*attempt;
}
}
