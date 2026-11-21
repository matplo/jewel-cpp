! Standalone PYEXEC for a complete outgoing colour graph.
program paired_hadronization
  implicit none
  integer n,npad,k(23000,5),mstu(200),mstj(200),mrpy(6)
  integer mdcy(500,3),mdme(8000,2),kfdp(8000,5),pycomp
  real(8) p(23000,5),v(23000,5),paru(200),parj(200),rrpy(100),brat(8000)
  common /pyjets/ n,npad,k,p,v
  common /pydat1/ mstu,paru,mstj,parj
  common /pydat3/ mdcy,mdme,brat,kfdp
  common /pydatr/ mrpy,rrpy
  integer event_id,i,j,ios,colour,anticolour
  mstj(11)=5
  mstj(22)=2
  parj(1)=0.073
  parj(2)=0.2
  parj(3)=0.94
  parj(4)=0.032
  parj(11)=0.31
  parj(12)=0.4
  parj(13)=0.54
  parj(25)=0.63
  parj(26)=0.12
  parj(21)=0.325
  parj(41)=0.5
  parj(42)=0.6
  parj(47)=0.67
  mdcy(pycomp(111),1)=1
  mstu(11)=0
  mstu(21)=1
  mrpy(1)=2600421
  mrpy(2)=0
  do
    read(*,*,iostat=ios) event_id,n
    if(ios<0)exit
    if(ios/=0.or.n<1.or.n>20000)stop 2
    do i=1,n
      read(*,*) k(i,2),colour,anticolour,p(i,1:5),v(i,1:4)
      if(colour<0.or.colour>n.or.anticolour<0.or.anticolour>n)stop 3
      k(i,1)=1
      if(colour/=0.or.anticolour/=0)k(i,1)=3
      k(i,3)=0
      k(i,4)=mstu(5)*colour
      k(i,5)=mstu(5)*anticolour
      v(i,5)=0
    enddo
    mstu(24)=0
    call pyexec
    write(*,*) 'EVENT',event_id,mstu(24),count(k(1:n,1)>=1.and.k(1:n,1)<=10)
    do j=1,n
      if(k(j,1)>=1.and.k(j,1)<=10)write(*,*) k(j,2),p(j,1:4)
    enddo
  enddo
end program
