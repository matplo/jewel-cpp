// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/pythia8_shower_output.hpp"
#include "jewel/strings.hpp"
#include <map>
#include <limits>
#include <stdexcept>
namespace jewel {
std::vector<GeneratorParticle> prepare_vacuum_hadronization(const ShowerState& state,bool recoils,bool compress){
 state.validate();if(state.discarded)throw std::domain_error("discarded hadronization input");
 std::vector<CompressionRecord> records;
 for(const auto& s:state.particles()){
  const auto& p=s.particle;
  if(p.status==1||p.status==2)throw std::domain_error("unfinished shower before hadronization");
  CompressionRecord r;r.k={p.status,p.pdg,0,0,0};if(!recoils&&p.status==3)r.k[0]=22;
  r.p={p.momentum.px,p.momentum.py,p.momentum.pz,p.momentum.energy,s.record_mass};
  r.v={s.generator_vertex.x,s.generator_vertex.y,s.generator_vertex.z,s.generator_vertex.t,0};
  r.production={p.production.x,p.production.y,p.production.z,p.production.t,s.decay_time};
  r.colour=p.colour;r.anticolour=p.anticolour;records.push_back(r);
 }
 auto result=make_vacuum_strings(records,records.size(),compress);
 if(result.discarded)throw std::domain_error("JEWEL vacuum string reconstruction discarded event");
 if(!result.unconsumed_sources.empty())throw std::domain_error("JEWEL vacuum string reconstruction left unconsumed partons");
 std::vector<GeneratorParticle> output;std::map<int,std::pair<int,int>> colours;
 for(auto slot:result.output_sources){const auto& s=state.particles().at(slot);const auto& p=s.particle;
  GeneratorParticle out;out.id=p.id;out.source_index=static_cast<int>(slot);out.pdg=p.pdg;out.status=23;out.final=true;
  out.colour=p.colour;out.anticolour=p.anticolour;out.momentum=p.momentum;out.mass=s.record_mass;
  out.vertex_mm={p.production.x*1e-12,p.production.y*1e-12,p.production.z*1e-12,p.production.t*1e-12};
  if(out.colour)++colours[out.colour].first;if(out.anticolour)++colours[out.anticolour].second;output.push_back(out);
 }
 // This HADROTYPE=0 adapter has no junction interpretation. Reject ambiguous
 // or open tags rather than asking Pythia to invent a different topology.
 for(const auto& entry:colours)if(entry.second!=std::pair<int,int>{1,1})throw std::domain_error("unbalanced or repeated hadronization colour tag");
 return output;
}
std::vector<GeneratorParticle> prepare_native_hadronization(const ShowerState& state,
 const std::vector<GeneratorJunction>& junctions,bool recoils){
 state.validate();if(state.discarded)throw std::domain_error("discarded native hadronization input");
 std::vector<GeneratorParticle> output;std::map<int,std::pair<int,int>> colours;int slot=-1;
 for(const auto& node:state.particles()){
  ++slot;const auto& p=node.particle;
  if(p.status==1||p.status==2)throw std::domain_error("unfinished native shower before hadronization");
  if((p.status!=3&&p.status!=4&&p.status!=5)||(!recoils&&p.status==3))continue;
  GeneratorParticle out;out.id=p.id;out.source_index=slot;out.pdg=p.pdg;out.status=23;out.final=true;
  out.colour=p.colour;out.anticolour=p.anticolour;out.momentum=p.momentum;out.mass=node.record_mass;
  out.vertex_mm={p.production.x*1e-12,p.production.y*1e-12,p.production.z*1e-12,p.production.t*1e-12};
  if(out.colour)++colours[out.colour].first;if(out.anticolour)++colours[out.anticolour].second;output.push_back(out);
 }
 for(const auto& j:junctions){
  if(j.kind!=1&&j.kind!=2)throw std::domain_error("native boundary requires outgoing junction kind 1 or 2");
  for(auto tag:j.colours){if(tag<=0)throw std::domain_error("invalid native junction colour");
   if(j.kind==1)++colours[tag].second;else ++colours[tag].first;
  }
 }
 for(const auto& entry:colours)if(entry.second!=std::pair<int,int>{1,1})throw std::domain_error("unbalanced native colour/junction tag");
 if(output.empty())throw std::domain_error("empty native hadronization input");
 return output;
}
std::vector<GeneratorParticle> prepare_minimum_hadronization(const ShowerState& state,
 const BranchingSampler& sampler,Random& random,double eta,bool recoils,bool compress){
 state.validate();if(state.discarded)throw std::domain_error("discarded hadronization input");
 std::map<ParticleId,int> slots;int slot=0;
 for(const auto& node:state.particles())slots[node.particle.id]=++slot;
 std::vector<CompressionRecord> records;
 for(const auto& node:state.particles()){
  const auto& p=node.particle;
  if(p.status==1||p.status==2)throw std::domain_error("unfinished shower before hadronization");
  CompressionRecord r;r.k={p.status,p.pdg,slots.count(node.parent)?slots.at(node.parent):0,0,0};
  // Native vacuum state can retain a nonphysical padding row. The Fortran
  // physical PYJETS input has no corresponding colourless particle to copy.
  if(p.status==0&&p.pdg==0&&p.colour==0&&p.anticolour==0&&p.momentum.px==0&&p.momentum.py==0&&p.momentum.pz==0&&p.momentum.energy==0)r.k[0]=17;
  if(!recoils&&p.status==3)r.k[0]=22;
  r.p={p.momentum.px,p.momentum.py,p.momentum.pz,p.momentum.energy,node.record_mass};
  r.v={node.generator_vertex.x,node.generator_vertex.y,node.generator_vertex.z,node.generator_vertex.t,0};
  r.production={p.production.x,p.production.y,p.production.z,p.production.t,node.decay_time};
  r.colour=p.colour;r.anticolour=p.anticolour;records.push_back(r);
 }
 auto result=make_minimum_strings(records,records.size(),compress,eta,sampler,random);
 if(result.discarded)throw std::domain_error("JEWEL minimum string reconstruction discarded event");
 // Ordered status-2 chains end at status 1; status 4 is a colourless copy.
 auto endpoint_type=[](int id){int a=std::abs(id),sign=id>0?1:-1;
  if(a>=1&&a<=6)return sign;
  if(a>=1000&&a<10000&&(a/10)%10==0)return -sign;
  throw std::domain_error("unsupported minimum-string endpoint");
 };
 std::vector<GeneratorParticle> output;int tag=100,open=0,orientation=0;
 for(std::size_t i=0;i<result.active;++i){const auto& r=records[i];
  GeneratorParticle p;p.id=i+1;p.pdg=r.k[1];p.status=23;p.final=true;
  p.momentum={r.p[0],r.p[1],r.p[2],r.p[3]};p.mass=r.p[4];
  auto source=result.output_sources.at(i);
  p.source_index=source==std::numeric_limits<std::size_t>::max()?-1:static_cast<int>(source);
  if(p.source_index>=0){const auto& v=state.particles().at(source).particle.production;
   p.vertex_mm={v.x*1e-12,v.y*1e-12,v.z*1e-12,v.t*1e-12};
   const auto& original=state.particles().at(source).particle;
   // MINL copies parent P(5) to both scaled quark halves. Pythia8 requires
   // a mass field consistent with the unchanged, scaled four-momentum.
   if(original.pdg==21&&std::abs(p.pdg)==1){
    if(original.momentum.energy<=0)throw std::domain_error("invalid split-gluon energy");
    p.mass*=p.momentum.energy/original.momentum.energy;
   }
  }
  if(r.k[0]==4){if(open)throw std::domain_error("colourless particle inside minimum string");}
  else if(r.k[0]==2){
   if(!open){orientation=endpoint_type(p.pdg);open=++tag;if(orientation>0)p.colour=open;else p.anticolour=open;}
   else {if(p.pdg!=21)throw std::domain_error("non-gluon inside minimum string");
    int next=++tag;if(orientation>0){p.anticolour=open;p.colour=next;}else{p.colour=open;p.anticolour=next;}open=next;}
  }else if(r.k[0]==1){
   if(!open||endpoint_type(p.pdg)!=-orientation)throw std::domain_error("unbalanced minimum-string endpoints");
   if(orientation>0)p.anticolour=open;else p.colour=open;open=0;
  }else throw std::domain_error("unsupported minimum-string output status");
  output.push_back(p);
 }
 if(open)throw std::domain_error("unterminated minimum string");
 return output;
}

}
