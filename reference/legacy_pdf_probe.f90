! Full frozen PYPDFU plus the PYSIGH heavy-flavour suppression.
program probe
implicit none
integer mstp(200),msti(200),mint(400),mstu(200),mstj(200),beam,nf,i,ios
real(8) parp(200),pari(200),vint(400),paru(200),parj(200),x,remaining,q2,charm,bottom,xpq(-25:25)
real(8) xmin,xmax,q2min,q2max
character(32) policy
common /W50513/ xmin,xmax,q2min,q2max
common /PYPARS/ mstp,parp,msti,pari
common /PYINT1/ mint,vint
common /PYDAT1/ mstu,paru,mstj,parj
mstp(51)=13100
mstp(52)=2
mstp(57)=1
mint(30)=0
mint(31)=0
mint(93)=0
! Keep PDF library messages off the numerical stream.
mstu(11)=0
! Initialize the external set before inspecting its COMMON state.
call pypdfu(2212,0.1d0,10d0,xpq)
call get_command_argument(1,policy)
if(trim(policy)=='metadata')then
 ! Explicit harness initialization; the PYPDFU body remains unchanged.
 call getq2min(0,q2min)
else if(len_trim(policy)/=0)then
 stop 2
endif
write(0,*) 'PDF_FLOOR',q2min
do
 read(*,*,iostat=ios) beam,x,remaining,q2,nf,charm,bottom
 if(ios/=0)exit
 mstp(58)=nf
 call pypdfu(beam,x/remaining,q2,xpq)
 if(q2<1.1*charm**2)then
  xpq(4)=0
  xpq(-4)=0
 endif
 if(q2<1.1*bottom**2)then
  xpq(5)=0
  xpq(-5)=0
 endif
 write(*,'(11ES25.17)') (xpq(i),i=-5,-1),xpq(21),(xpq(i),i=1,5)
enddo
end program
