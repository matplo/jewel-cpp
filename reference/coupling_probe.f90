subroutine coupling_probe
implicit none
integer mstu(200),mstj(200),mstp(200),msti(200),i,channel
real(8) paru(200),parj(200),parp(200),pari(200),q(10),pyalps,x
common /PYDAT1/ mstu,paru,mstj,parj
common /PYPARS/ mstp,parp,msti,pari
q=[1d0,2d0,5d0,10d0,20d0,50d0,91.1876d0,100d0,200d0,1000d0]
write(*,*)'COUPLING_CONFIG',mstp(2),mstp(3),mstu(111:115),parp(1),parp(61),parp(64),mstp(64)
do channel=1,2
 paru(112)=parp(1)
 if(channel==2)paru(112)=parp(61)
 do i=1,10
  x=pyalps(q(i)*q(i))
  write(*,*)'COUPLING',channel,q(i),x,mstu(118),paru(117)
 enddo
enddo
end subroutine
