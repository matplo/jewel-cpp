! SPDX-License-Identifier: GPL-2.0-or-later
program probe
 implicit none
 double precision Q0,LPS,LQCD,LTIME,SCALEFACM,RECSOFTCUT,RECHARDCUT
 integer ANGORD,NF,KINMODE,recmode
 logical SCATRECOIL,ALLHAD,compress,mpifsr,aofirstsplit
 common /PARAM/ Q0,LPS,LQCD,LTIME,SCALEFACM,RECSOFTCUT,RECHARDCUT, &
 ANGORD,aofirstsplit,SCATRECOIL,ALLHAD,compress,mpifsr,NF,KINMODE,recmode
 double precision tape(1000000),a,b,c,value
 integer n,cursor,ios
 common /REPLAY/ tape,n,cursor
 double precision PQQ,PGQ,PGG,PQG,ALPHAS,INTPQQ,INTPGGLOW,INTPGGHIGH,GETT
 character(20) op
 LPS=0.2d0
 LQCD=0.2d0
 do
  read(*,*,iostat=ios) op,nf,a,b,c
  if(ios<0) exit
  if(ios/=0) stop 2
  cursor=0
  select case(trim(op))
  case('PQQ'); value=PQQ(a)
  case('PGQ'); value=PGQ(a)
  case('PGG'); value=PGG(a)
  case('PQG'); value=PQG(a)
  case('ALPHAS'); value=ALPHAS(a,b)
  case('INTPQQ'); value=INTPQQ(a,b)
  case('INTPGGLOW'); value=INTPGGLOW(a,b)
  case('INTPGGHIGH'); value=INTPGGHIGH(a,b)
  case('GETT')
   read(*,*) n
   if(n<0.or.n>1000000) stop 3
   if(n>0) read(*,*) tape(1:n)
   value=GETT(a,b,c)
  case default; stop 4
  end select
  write(*,'(ES26.17E3,1X,I0)') value,cursor
 end do
end program
function PYR(dummy) result(value)
 implicit none
 integer dummy,n,cursor
 double precision value,tape(1000000)
 common /REPLAY/ tape,n,cursor
 if(cursor>=n) stop 5
 cursor=cursor+1
 value=tape(cursor)
 if(value<=0.d0.or.value>=1.d0) stop 6
end function
