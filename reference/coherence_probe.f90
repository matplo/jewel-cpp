! Reference-only section driver. Not linked into the C++ library.
program coherence
 use, intrinsic :: ieee_arithmetic
 implicit none
 integer n,npad,k(23000,5),mstu(200),mstj(200),status,mode,i
 double precision p(23000,5),v(23000,5),paru(200),parj(200)
 common /PYJETS/ n,npad,k,p,v
 common /PYDAT1/ mstu,paru,mstj,parj
 double precision q(1,6),f,a,t,s,z,phi,qp,dp,pz,pts,threshold,first(4),second(4)
 logical resolved,invalid
 character(20) op
 mstu=0; mstu(4)=23000; paru=0; paru(1)=3.141592653589793d0
 do
  read(*,*,iostat=status)op
  if(status<0)exit
  if(status/=0)stop 2
  n=3;k=0;p=0;v=0;q=0;k(3,1)=1
  qp=0;dp=0;pz=0;pts=0;f=0;a=0;t=0;s=0;z=0;phi=0
  if(trim(op)=='GATE')then
   mode=1
   read(*,*)p(3,1:4),q(1,1:4),f,a,t,s
  else if(trim(op)=='SPLIT')then
   mode=2
   read(*,*)p(3,1:5),v(3,1:4),p(1,5),p(2,5),z,phi
  else
   stop 3
  endif
  call coherent_reference(mode,f,a,t,s,q,z,phi,qp,dp,pz,pts,resolved)
  if(mode==1)then
   invalid=.not.ieee_is_finite(qp)
   if(invalid)then
    write(*,*)1
   else
    threshold=0.2/dp
    write(*,'(I0,3ES26.17E3,1X,I0)')0,qp,dp,threshold,merge(1,0,resolved)
   endif
  else
   invalid=.not.ieee_is_finite(pz).or..not.ieee_is_finite(pts)
   if(invalid)then
    write(*,*)1
   else
    first=0;second=0
    if(n==5)then
     first=p(4,1:4);second=p(5,1:4)
    endif
    write(*,'(I0,1X,I0,18ES26.17E3)')0,merge(1,0,n==5),pz,pts,p(3,1:4),first,second,v(3,1:4)
   endif
  endif
 enddo
end program
