program projection
 implicit none
 integer n,npad,k(23000,5),mstu(200),mstj(200),i,ios,first,warnings
 double precision p(23000,5),v(23000,5),paru(200),parj(200),masses(23000)
 common /PYJETS/ n,npad,k,p,v
 common /PYDAT1/ mstu,paru,mstj,parj
 common /WARNINGS/ warnings
 common /TESTMASSES/ masses
 integer nscatcen,maxnscatcen,scatflav(23000),putonshell
 double precision scatcen(23000,5),rsub
 logical writescatcen,writedummies,dosubtraction
 common /storescatcen/ nscatcen,maxnscatcen,scatflav,scatcen,writescatcen,writedummies,dosubtraction,putonshell,rsub
 integer nsim,npart,offset,hadrotype
 double precision sqrts
 character*4 collider,channel
 character*2 isochannel
 logical hadro,shorthepmc,isrscat
 common /evrecord/ nsim,npart,offset,hadrotype,sqrts,collider,hadro,shorthepmc,isrscat,channel,isochannel
 integer hpmcfid,logfid
 common /hepmcid/ hpmcfid,logfid
 logfid=0;mstu=0;mstu(4)=23000;paru=0;paru(1)=3.141592653589793d0
 do
  read(*,*,iostat=ios)n,putonshell,first
  if(ios/=0)exit
  if(n<1.or.n>22990.or.first<0.or.first>1)stop 2
  collider='PPYJ'
  if(first==1)collider='PPZJ'
  k=0;p=0;v=0;warnings=0
  do i=1,n
   k(i,1)=1;k(i,2)=i
   read(*,*)p(i,:),masses(i)
  enddo
  call putparticlesonshell
  write(*,*)n
  do i=1,n
   write(*,'(5ES26.17E3)')p(i,:)
  enddo
 enddo
end program
subroutine pyerrm(code,message)
 implicit none
 integer code,warnings
 character(*) message
 common /WARNINGS/ warnings
 if(code/=3)stop 3
 warnings=warnings+1
end subroutine
integer function pychge(code)
 integer code
 stop 4
end function
double precision function pymass(code)
 implicit none
 integer code
 double precision masses(23000)
 common /TESTMASSES/ masses
 if(code<1.or.code>23000)stop 5
 pymass=masses(code)
end function
