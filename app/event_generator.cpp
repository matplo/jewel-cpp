// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/event_generator.hpp"
#include "jewel/pythia8_hadronizer.hpp"
#include "event_physics.hpp"

namespace jewel {
namespace {
constexpr double max_rapidity=2;

EventGeneratorConfig validated(EventGeneratorConfig config){
 if(config.mode!="vacuum"&&config.mode!="medium"&&config.mode!="zero-interaction")
  throw std::invalid_argument("mode must be vacuum, medium or zero-interaction");
 if(!std::isfinite(config.impact_parameter_fm)||config.impact_parameter_fm<0
    ||(config.mode=="vacuum"&&config.impact_parameter_fm!=0))
  throw std::invalid_argument("a nonzero impact parameter requires an expanding medium");
 if(config.input.multiple_parton_interactions&&!config.input.initial_state_radiation)
  throw std::invalid_argument("Pythia8 MPI requires ISR in this generator");
 if(config.angular_ordering<0||config.angular_ordering>5)
  throw std::invalid_argument("angular ordering must be in [0, 5]");
 return config;
}

MediumModel make_medium(const EventGeneratorConfig& config){
 if(config.mode=="vacuum")return {};
 MediumConfig medium;
 medium.max_rapidity=max_rapidity;
 medium.impact_parameter=config.impact_parameter_fm;
 return ExpandingMedium(medium);
}
}

struct EventGenerator::Impl {
 EventGeneratorConfig config;
 Pythia8Input input;
 Pythia8Hadronizer hadronizer;
 MediumModel medium;
 EventPhysics physics;
 InitialEvolutionConfig evolution;
 std::uint64_t attempts{0};

 explicit Impl(EventGeneratorConfig requested)
  : config(validated(std::move(requested))),input(config.input),
    hadronizer({config.input.xml_directory,config.input.seed,
                config.jewel_fragmentation?FragmentationProfile::Jewel260Mapped:FragmentationProfile::Monash14,
                config.pi0_decay,config.decay_tau0_max_mm}),
    medium(make_medium(config)),
    physics(medium,jewel_table_energy(config.input,max_rapidity),
            config.longitudinal_boost,config.angular_ordering) {
  evolution.evolution.cross_section_scale=config.mode=="medium"?1:0;
  evolution.coherent.cross_section_scale=evolution.evolution.cross_section_scale;
  evolution.coherent.finalization.hadronize_recoils=config.keep_recoils;
  evolution.coherent.finalization.scatter_recoils=false;
  evolution.coherent.finalization.soft_recoil_cut=0;
  evolution.coherent.finalization.hard_recoil_cut=0;
  evolution.coherent.finalization.storage={0,true,false};
 }
};

EventGenerator::EventGenerator(EventGeneratorConfig config):impl_(std::make_unique<Impl>(std::move(config))){}
EventGenerator::~EventGenerator()=default;
GeneratorStatistics EventGenerator::hard_statistics() const{return impl_->input.statistics();}

GeneratedEvent EventGenerator::next(){
 auto& run=*impl_;
 GeneratedEvent event;
 event.attempt=++run.attempts;
 auto hard=run.input.next();
 if(!hard){
  event.stage="hard input";event.error="Pythia8 next() failed";
  return event;
 }
 event.weight=hard->weight;event.pt_hat=hard->pt_hat;
 SplitMixRandom random(jewel_shower_seed(run.config.input.seed,event.attempt));
 const bool imported=run.config.input.initial_state_radiation;
 try{
  event.stage="geometry";
  auto vertex=run.medium.sample_vertex(random);
  double lifetime=run.medium.bounds().lifetime_max;
  auto prepared=run.physics.prepare(*hard,run.input,vertex,lifetime,max_rapidity,imported,random,event.stage);
  event.stage="cascade";
  RecoilStorage ledger;
  run.physics.evolve(prepared,run.medium,run.evolution,lifetime,std::nullopt,ledger,random);
  event.stage="string construction";
  auto strings=run.physics.strings(prepared.state,*hard,imported,run.config.keep_recoils,0,max_rapidity,random);
  event.stage="hadronization";
  auto hadrons=run.hadronizer.hadronize(strings,imported?hard->junctions:std::vector<GeneratorJunction>{});
  if(!hadrons)throw std::runtime_error("Pythia8 hadronization failed");
  for(const auto& p:hadrons->particles)if(p.final)event.particles.push_back({p.pdg,p.momentum});
  event.accepted=true;
  event.stage.clear();
 }catch(const std::domain_error& error){event.error=error.what();}
  catch(const std::runtime_error& error){event.error=error.what();}
 return event;
}

}
