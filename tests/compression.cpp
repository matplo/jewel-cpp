// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/compression.hpp"
#include "jewel/shower_state.hpp"
#include <stdexcept>
void check(bool value){if(!value)throw std::runtime_error("compression contract failed");}
template<class F>void rejects(F f){bool caught=false;try{f();}catch(const std::domain_error&){caught=true;}check(caught);}
int main(){
 // Native history survives compaction while source scheduling links clear.
 {
  jewel::ShowerState state;
  jewel::ShowerParticle node;node.particle.status=0;auto scratch=state.append(node);
  node.particle.status=11;node.evolution_time=11;auto mother=state.append(node);
  node.parent=mother;node.particle.status=1;node.evolution_time=22;node.medium_induced=true;
  auto first=state.append(node);node.evolution_time=33;auto second=state.append(node);state.connect(mother,first,second);
  auto before=state.snapshot();auto result=state.compress(first);
  state.validate();check(result.active==3&&state.archived().size()==1);
  check(state.particles()[1].particle.id==first&&state.at(first).parent==mother);
  check(state.at(first).particle.parents==std::vector<jewel::ParticleId>{mother});
  check(state.at(mother).daughters==std::array<jewel::ParticleId,2>{first,second});
  check(!state.scheduling_parent(first)&&!state.scheduling_parent(second));
  check(state.at(first).evolution_time==11&&!state.at(first).medium_induced);
  check(state.at(second).evolution_time==22&&state.at(second).medium_induced);
  auto saved=state.snapshot();state.restore(saved);state.validate();
  auto corrupt=saved;corrupt.archived.at(mother).parent=first;
  corrupt.archived.at(mother).particle.parents={first};
  rejects([&]{state.restore(corrupt);});state.validate();
  check(state.archived().size()==1&&state.at(mother).particle.status==11);
  node.parent=first;node.particle.status=4;node.daughters={};
  auto a=state.append(node,true,true),b=state.append(node,true,true);state.connect(first,a,b);
  check(a>second&&state.scheduling_daughters(first)==std::array<jewel::ParticleId,2>{a,b});
  check(state.at(a).evolution_time==33);state.validate();
  auto again=state.compress(scratch);check(again.removed==0);state.validate();
  state.at(first).particle.status=11;
  auto repeated=state.compress(second);check(repeated.removed==1&&state.archived().size()==2);
  check(state.at(a).parent==first&&state.at(first).parent==mother&&!state.scheduling_parent(a));
  state.validate();
  state.restore(before);check(state.archived().empty()&&state.particles().size()==4);
 }
 std::vector<jewel::CompressionRecord> records(15);
 for(std::size_t i=0;i<records.size();++i){auto& r=records[i];r.k={11,static_cast<int>(i),1,2,3};r.evolution_time=i;r.medium_induced=i%2;}
 records[13].k[0]=1;
 auto result=jewel::compress_records(records,14);
 check(result.active==3&&result.selected==2&&result.removed==12&&result.room_left);
 check(result.source_slots==std::vector<std::size_t>{0,13,14});
 check(records[0].k[2]==1&&records[1].k[1]==13&&records[2].k[1]==14);
 check(records[1].k[2]==0&&records[2].k[3]==0);
 check(records[1].evolution_time==1&&records[2].evolution_time==2&&records[1].medium_induced);
 // Repeated compression must use the active prefix, not resurrect cleared slots.
 auto again=jewel::compress_records(records,result.active,result.selected);
 check(again.active==3&&!again.room_left&&again.removed==0);
 auto unprotected=jewel::compress_records(records,again.active,again.active);
 check(unprotected.active==2&&unprotected.selected==3&&unprotected.removed==1);
 check(records[14].evolution_time==14&&records.size()==15);
 rejects([&]{jewel::compress_records(records,0,0);});
 rejects([&]{jewel::compress_records(records,16,0);});
 rejects([&]{jewel::compress_records(records,2,3);});
 std::vector<jewel::CompressionRecord> empty;
 rejects([&]{jewel::compress_records(empty,0);});
}
