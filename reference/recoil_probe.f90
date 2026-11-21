program probe
 implicit none
 integer mode,iw,isub,isoft,ireject,used,capacity,flavour,ios,i,j,k(3,5),sf(23000)
 double precision p(3,5),sc(23000,5),d(23000,5)
 do
  p=0; k=0;sf=-99
  do j=1,5
   sc(:,j)=dble(j);d(:,j)=dble(j+5)
  enddo
  read(*,*,iostat=ios)mode,iw,isub,isoft,ireject,used,capacity,flavour,p(1,:),p(2,1:4)
  if(ios<0)exit
  if(ios/=0)stop 2
  k(1,2)=flavour
  call recoil_reference(p,k,3,sc,d,sf,used,capacity,mode,iw/=0,isub/=0,ireject/=0,isoft/=0)
  write(*,'(*(es26.17e3,1x))')dble(used),(dble(sf(i)),sc(i,:),d(i,:),i=1,capacity)
 enddo
end program
