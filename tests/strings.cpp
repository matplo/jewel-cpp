// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/strings.hpp"
#include <stdexcept>
void check(bool value){if(!value)throw std::runtime_error("string contract failed");}
int main(){
 using jewel::CompressionRecord;std::vector<CompressionRecord> rows(4);
 rows[1].k={4,1,0,0,0};rows[1].colour=101;rows[1].p={1,0,0,1,0};
 rows[2].k={4,21,0,0,0};rows[2].colour=102;rows[2].anticolour=101;rows[2].p={0,1,0,1,0};
 rows[3].k={4,-1,0,0,0};rows[3].anticolour=102;rows[3].p={-1,-1,0,2,0};
 auto original=rows;auto result=jewel::make_vacuum_strings(rows,rows.size(),false);
 check(!result.discarded&&result.active==3&&result.output_sources==std::vector<std::size_t>{1,2,3});
 check(result.unconsumed_sources.empty());check(rows[0].k[0]==2&&rows[2].k[0]==1);
 // CLEANUP copies K/P/V but leaves the physical colour slot at scratch's zero.
 check(rows[0].colour==0&&rows[0].p==original[1].p);
 rows=original;rows[3].anticolour=999;result=jewel::make_vacuum_strings(rows,rows.size(),false);check(result.discarded);
 rows.assign(3,{});rows[1].k={4,21,0,0,0};rows[1].colour=101;rows[1].anticolour=102;
 rows[2].k={4,21,0,0,0};rows[2].colour=102;rows[2].anticolour=101;
 result=jewel::make_vacuum_strings(rows,rows.size(),false);
 check(!result.discarded&&result.active==0&&result.unconsumed_sources==std::vector<std::size_t>{1,2});
}
