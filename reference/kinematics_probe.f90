! Reference-only probe; never linked into the C++ engine.
program kinematics
 implicit none
 integer hpmcfid,logfid
 common /hepmcid/ hpmcfid,logfid
 integer n,npad,k(23000,5),mstu(200),mstj(200),i,status,warnings
 double precision p(23000,5),v(23000,5),paru(200),parj(200),mv(23000,5)
 common /PYJETS/ n,npad,k,p,v
 common /PYDAT1/ mstu,paru,mstj,parj
 common /TIME/ mv
 common /WARNINGS/ warnings
 double precision theta,phi,bx,by,bz,pyp,mass,time,x,y,z
 character(20) op
 logfid=0
 mstu=0
 mstu(4)=23000
 paru=0
 paru(1)=3.141592653589793d0
 n=1
 k=0
 p=0
 v=0
 do
  read(*,*,iostat=status)op
  if(status<0)exit
  if(status/=0)stop 2
  select case(trim(op))
  case('MASSLESS')
   n=3
   do i=1,3
    read(*,*)p(i,1:5),v(i,1:4)
    k(i,1)=1
   enddo
   call makemassless(3,1,2)
   do i=1,3
    write(*,'(9ES26.17E3)')p(i,1:5),v(i,1:4)
   enddo
   n=1
  case('TRANSFORM')
   read(*,*)p(1,1:4),v(1,1:4),theta,phi,bx,by,bz
   k(1,1)=1
   warnings=0
   call pyrobo(1,1,theta,phi,bx,by,bz)
   write(*,'(8ES26.17E3,1X,I0)')p(1,1:4),v(1,1:4),warnings
  case('OBSERVABLES')
   read(*,*)p(1,1:5)
   write(*,'(5ES26.17E3)')pyp(1,8),pyp(1,13),pyp(1,15),pyp(1,17),pyp(1,19)
  case('POSITION')
   read(*,*)p(1,1:4),mv(1,1:4),time
   call getposition(1,time,x,y,z)
   write(*,'(3ES26.17E3)')x,y,z
  case default
   stop 3
  end select
 enddo
end program
subroutine pyerrm(code,message)
 implicit none
 integer code,warnings
 character(*) message
 common /WARNINGS/ warnings
 if(code/=3)stop 4
 warnings=warnings+1
end subroutine
integer function pychge(code)
 implicit none
 integer code
 stop 5 ! Charge lookup is outside this probe's tested paths.
end function
double precision function pymass(code)
 implicit none
 integer code
 stop 6 ! Pion-mass rapidity is outside this probe's tested paths.
end function
