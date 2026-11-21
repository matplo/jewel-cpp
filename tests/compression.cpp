// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/compression.hpp"
#include <stdexcept>
void check(bool value){if(!value)throw std::runtime_error("compression contract failed");}
template<class F>void rejects(F f){bool caught=false;try{f();}catch(const std::domain_error&){caught=true;}check(caught);}
int main(){
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
