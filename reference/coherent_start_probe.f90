program coherent_start
 implicit none
 integer n,npad,k(23000,5),mstu(200),mstj(200),status,l,enabled,i,pair,dir
 integer trip(23000),anti(23000)
 double precision p(23000,5),v(23000,5),paru(200),parj(200),mv(23000,5)
 double precision zd(23000),thetad(23000),start,duration,angle,deltat,tleft,z,phi
 logical qqbard(23000),active,quark,qqbar
 common /PYJETS/ n,npad,k,p,v
 common /PYDAT1/ mstu,paru,mstj,parj
 mstu=0;mstu(4)=23000;paru=0;paru(1)=3.141592653589793d0
 do
  read(*,*,iostat=status)l,enabled,start,duration,angle
  if(status<0)exit
  if(status/=0)stop 2
  n=4;k=0;p=0;v=0;mv=0;zd=0;thetad=0;qqbard=.false.;trip=0;anti=0
  do i=1,4
   read(*,*)k(i,:),p(i,:),v(i,1:4),mv(i,:),zd(i),thetad(i),pair,trip(i),anti(i)
   qqbard(i)=pair/=0
  enddo
  call coherent_start_reference(l,enabled/=0,start,duration,angle,mv,trip,anti, &
   zd,thetad,qqbard,active,deltat,tleft,z,phi,quark,qqbar,dir)
  if(active)then
   write(*,'(I0,4ES26.17E3,4(1X,I0))')1,deltat,tleft,z,phi,merge(1,0,quark),merge(1,0,qqbar),dir,n
  else
   write(*,'(I0,ES26.17E3,1X,I0)')0,deltat,n
  endif
  do i=1,n
   write(*,'(5(I0,1X),16ES26.17E3,3(1X,I0))')k(i,:),p(i,:),v(i,1:4),mv(i,:), &
    zd(i),thetad(i),merge(1,0,qqbard(i)),trip(i),anti(i)
  enddo
 enddo
end program
