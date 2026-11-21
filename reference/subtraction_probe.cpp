// Calls unchanged source helpers with an explicit zero offset. The upstream
// Fortran-facing wrapper's process-name bug is deliberately outside this test.
#include <iomanip>
#include <cstdlib>
#include "subtraction.C"
int main(){
 static int k[5][23000];static double p[5][23000],thermal[5][23000];
 int n,m;unsigned seed;double radius;std::cout<<std::setprecision(17);
 while(std::cin>>n>>m>>radius>>seed){
  if(n<0||n>=23000||m<0||m>=23000)return 2;
  for(int i=0;i<n;++i){k[0][i]=1;std::cin>>k[1][i];for(int j=0;j<5;++j)std::cin>>p[j][i];}
  for(int i=0;i<m;++i)for(int j=0;j<4;++j)std::cin>>thermal[j][i];
  if(!std::cin)return 2;
  std::srand(seed);auto particles=readEvent(k,p,n,0);auto thermals=readThermals(thermal,m);
  // Avoid upstream's undefined &empty_vector[0]; all nonempty helper bodies
  // remain unchanged. Empty-pair behavior is covered by native contracts.
  std::vector<MyDist> pairs;if(!particles.empty()&&!thermals.empty())pairs=buildPairs(&particles,&thermals);
  doSubtraction(&pairs,&particles,&thermals,radius);collectResult(&particles,&thermals,k,p,&n,0);
  std::cout<<n<<'\n';for(int i=0;i<n;++i){std::cout<<k[1][i];for(int j=0;j<5;++j)std::cout<<' '<<p[j][i];std::cout<<'\n';}
 }
}
