! SPDX-License-Identifier: GPL-2.0-or-later
! Only DERIV dispatch (modes 5/101) and initial COMMON configuration are harness code.
program mediumprobe
 implicit none
 double precision centrmin,centrmax,breal,centr,rau
 integer nf
 common /MEDPARAM/ centrmin,centrmax,breal,centr,rau,nf
 double precision taui,ti,tc,d3,zeta3,d,n0,sigmann
 integer mass
 logical woods
 common /MEDPARAMINT/ taui,ti,tc,d3,zeta3,d,n0,sigmann,mass,woods
 double precision mdfactor,mdscalefac,etamax2,rmax,ta(100,2)
 common /MDFAC/ mdfactor,mdscalefac
 common /rapmax2/ etamax2
 common /THICKFNC/ rmax,ta
 integer logfid,hpmcfid,logfid2
 common /logfile/ logfid
 common /hepmcid/ hpmcfid,logfid2
 double precision eixs(3,1000),valmax
 integer nval
 common /EXPINT/ eixs,valmax,nval
 double precision x,y,z,t, gettemp,getmd,getms,getneff,nthick,npart,ei,v
 integer ios,i,woodflag
 double precision Q0,LPS,LQCD,LTIME,SCALEFACM,RECSOFTCUT,RECHARDCUT
 integer ANGORD,KNF,KINMODE,recmode
 logical SCATRECOIL,ALLHAD,compress,mpifsr,aofirstsplit
 common /PARAM/ Q0,LPS,LQCD,LTIME,SCALEFACM,RECSOFTCUT,RECHARDCUT, &
 ANGORD,aofirstsplit,SCATRECOIL,ALLHAD,compress,mpifsr,KNF,KINMODE,recmode
 double precision impmax,cross(200,3),intpqglow,intpqghigh,scatprimfunc
 common /CROSSSEC/ impmax,cross
 character(20) op
 logfid=0
 logfid2=0
 d3=0.9d0
 zeta3=1.2d0
 call eixint
 do
  read(*,*,iostat=ios) op
  if(ios<0) exit
  if(ios/=0) stop 2
  select case(trim(op))
  case('CONFIG')
   read(*,*) mass,nf,woodflag,taui,ti,tc,n0,d,sigmann,breal,etamax2,mdfactor,mdscalefac
   woods=woodflag/=0
   call calcta
  case('CROSS')
   call calcxsection
   do i=1,200
    write(*,'(3ES26.17E3)') cross(i,:)
   enddo
  case('LOW','HIGH','SCAT')
   read(*,*) knf,x,y,lps,lqcd
   if(trim(op)=='LOW') v=intpqglow(x,y)
   if(trim(op)=='HIGH') v=intpqghigh(x,y)
   if(trim(op)=='SCAT') v=scatprimfunc(x,y)
   write(*,'(ES26.17E3)') v
  case('TABLE')
   do i=1,100
    write(*,'(2ES26.17E3)') ta(i,1),ta(i,2)
   enddo
  case('EITABLE')
   do i=1,1000
    write(*,'(3ES26.17E3)') eixs(:,i)
   enddo
  case('EI')
   read(*,*) x
   v=ei(x)
   write(*,'(ES26.17E3)') v
  case('POINT')
   read(*,*) x,y,z,t
   write(*,'(4ES26.17E3)') gettemp(x,y,z,t),getmd(x,y,z,t),getms(x,y,z,t),getneff(x,y,z,t)
  case('THICK')
   read(*,*) x,y
   write(*,'(ES26.17E3)') nthick(x,y)
  case('PART')
   read(*,*) x,y,z,t
   write(*,'(ES26.17E3)') npart(x,y,z,t)
  case default
   stop 3
  end select
 enddo
end program
function deriv(x,w) result(v)
 implicit none
 double precision x,v,medderiv
 integer w
 if(w==5) then
  v=exp(-x)/x
 else if(w==101) then
  v=medderiv(x,w-100)
 else
  stop 4
 endif
end function
