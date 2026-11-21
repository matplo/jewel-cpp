program probe
 implicit none
 integer n,k(23000,5),i,ios,cursor,ntape,enabled
 double precision p(23000,5),mv(23000,5),masses(23000),ftfac,x0,y0,ltime,tape(10000)
 logical isrscat
 common /input/ n,k,p,mv,masses,ftfac,x0,y0,ltime,isrscat
 common /rng/ tape,cursor,ntape
 do
  read(*,*,iostat=ios)n,ftfac,x0,y0,ltime,enabled,ntape
  if(ios/=0)exit
  isrscat=enabled/=0
  if(n<106.or.n>23000.or.ntape>10000)stop 2
  k=0;p=0;mv=0;masses=0;cursor=0
  do i=105,n
   read(*,*)k(i,3),p(i,:),masses(i)
   k(i,2)=i
  enddo
  read(*,*)tape(:ntape)
  call isrtimes
  write(*,*)cursor
  do i=105,n
   write(*,'(5ES26.17E3)')mv(i,:)
  enddo
 enddo
end program
function pyr(dummy) result(value)
 implicit none
 integer dummy,cursor,ntape
 double precision tape(10000),value
 common /rng/ tape,cursor,ntape
 cursor=cursor+1
 if(cursor>ntape)stop 3
 value=tape(cursor)
end function
function pyp(i,j) result(value)
 implicit none
 integer i,j,n,k(23000,5)
 double precision p(23000,5),mv(23000,5),masses(23000),ftfac,x0,y0,ltime,value
 logical isrscat
 common /input/ n,k,p,mv,masses,ftfac,x0,y0,ltime,isrscat
 if(j/=8)stop 4
 value=sqrt(p(i,1)**2+p(i,2)**2+p(i,3)**2)
end function
function pymass(i) result(value)
 implicit none
 integer i,n,k(23000,5)
 double precision p(23000,5),mv(23000,5),masses(23000),ftfac,x0,y0,ltime,value
 logical isrscat
 common /input/ n,k,p,mv,masses,ftfac,x0,y0,ltime,isrscat
 value=masses(i)
end function
