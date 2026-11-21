program probe
 implicit none
 integer n,npad,k,nold,selected,i,j,ios,iq,ia,im,hpmcfid,logfid,colmax,trip,anti
 double precision p,v,mv,zd,thetad,starttime
 logical qqbard,aoreq,medind,room,compressevent
 common /pyjets/n,npad,k(23000,5),p(23000,5),v(23000,5)
 common /angor/zd(23000),thetad(23000),qqbard(23000),aoreq(23000)
 common /time/mv(23000,5)
 common /colour/trip(23000),anti(23000),colmax
 common /hepmcid/hpmcfid,logfid
 dimension starttime(23000),medind(23000)
 logfid=0
 do
  read(*,*,iostat=ios)n,selected
  if(ios/=0)exit
  nold=n
  if(selected==n)then
   selected=0
  else
   selected=selected+1
  endif
  do i=1,n
   read(*,*)k(i,:),p(i,:),v(i,:),mv(i,:),trip(i),anti(i),zd(i),thetad(i),iq,ia,starttime(i),im
   qqbard(i)=iq/=0;aoreq(i)=ia/=0;medind(i)=im/=0
  enddo
  room=compressevent(selected)
  if(selected==0)then
   selected=nold
  else
   selected=selected-1
  endif
  write(*,*)n,selected,merge(1,0,room)
  do i=1,nold
   write(*,*)k(i,:),p(i,:),v(i,:),mv(i,:),trip(i),anti(i),zd(i),thetad(i), &
    merge(1,0,qqbard(i)),merge(1,0,aoreq(i)),starttime(i),merge(1,0,medind(i))
  enddo
 enddo
end program
