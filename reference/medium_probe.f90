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
 double precision tape(100000),px,py,pz,energy,ms
 integer count,cursor,boostflag,ptype
 common /REPLAY/ tape,count,cursor
 logical boost
 common /boostmed/ boost
 double precision gettempmax,getmdmax,getmdmin,getmsmax,getnatmdmin,getltimemax,getneffmax
 double precision getinspliti,getinsudakov,getinsudared,getinsudafast,emax
 double precision sudaqq(1000,2),sudaqg(1000,2),sudagg(1000,2),sudagc(1000,2)
 common /INSUDA/ sudaqq,sudaqg,sudagg,sudagc
 character(2) channel
 double precision splitiggv(1000,1000),splitiqqv(1000,1000),splitiqgv(1000,1000),qval(1000),zmval(1000),qmax,zmmin
 integer npoint,j
 common /SPLITINT/ splitiggv,splitiqqv,splitiqgv,qval,zmval,qmax,zmmin,npoint
 double precision getspliti,getsudakov,qa1,qb1,za1,eb1,high
 character(1024) dumpfile
 double precision qinqx(2,1000),ginqx(2,1000),qingx(2,1000),gingx(2,1000)
 common /PDFS/ qinqx,ginqx,qingx,gingx
 double precision getpdf,getpdfxintexact,getpdfxint
 double precision intq1(1001,101),intq2(1001,101),intg1(1001,101),intg2(1001,101)
 common /XSECS/ intq1,intq2,intg1,intg2
 double precision getxsecint,getsscat,pmass,lower
 integer rate_mode
 character incoming,outgoing
 double precision ptfac,getmass,generatez,zdec,angle,maxmass
 common /ALPHASFAC/ ptfac
 integer initial_flag
 logical qqbardec
 integer n,npad,k(23000,5),errl,elastic_flag
 double precision p(23000,5),vertices(23000,5),mv(23000,5),xsecscalefac,etamax
 double precision start,dt,elapsed
 common /PYJETS/ n,npad,k,p,vertices
 common /TIME/ mv
 common /errline/ errl
 common /scalfac/ xsecscalefac
 common /rapmax/ etamax
 logical getdeltat,collision
 double precision zd(23000),thetad(23000),starttime(23000),wdisc
 logical qqbard(23000),aoreq(23000),medind(23000),discard
 integer ndisc,nstrange,ngood,errcount,mother,qflag,aflag
 common /ANGOR/ zd,thetad,qqbard,aoreq
 common /evoltimes/ starttime,medind
 common /DISC/ ndisc,nstrange,ngood,errcount,wdisc,discard
 integer mstu(200),mstj(200)
 double precision paru(200),parj(200)
 common /PYDAT1/ mstu,paru,mstj,parj
 double precision ftfac,evweight,sumofweights
 integer trip(23000),anti(23000),colmax
 common /FTIMEFAC/ ftfac
 common /COLOUR/ trip,anti,colmax
 common /WEIGHT/ evweight,sumofweights
 double precision getnewmass,getpnorad1,qnew2,qold2,previousmass,energyfraction
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
  case('EVENT')
   read(*,*) centrmin,centrmax
   read(*,*) count
   if(count<1.or.count>100000) stop 5
   read(*,*) tape(1:count)
   cursor=0
   call calcxsection
   call mednextevt
   write(*,'(2ES26.17E3,1X,I0)') breal,centr,cursor
  case('VERTEX')
   read(*,*) breal,rau
   read(*,*) count
   if(count<1.or.count>100000) stop 5
   read(*,*) tape(1:count)
   cursor=0
   call pickvtx(x,y)
   write(*,'(2ES26.17E3,1X,I0)') x,y,cursor
  case('SCATTER')
   read(*,*) x,y,z,t,boostflag
   read(*,*) count
   if(count<1.or.count>100000) stop 5
   read(*,*) tape(1:count)
   cursor=0
   boost=boostflag/=0
   call getscatterer(x,y,z,t,ptype,px,py,pz,energy,ms)
   write(*,'(I0,5ES26.17E3,1X,I0)') ptype,px,py,pz,energy,ms,cursor
  case('BOUNDS')
   write(*,'(7ES26.17E3)') gettempmax(),getmdmax(),getmdmin(),getmsmax(),getnatmdmin(),getltimemax(),getneffmax()
  case('AVERAGE')
   read(*,*) x,y,z,t,boostflag
   boost=boostflag/=0
   call avscatcen(x,y,z,t,px,py,pz,energy,ms)
   write(*,'(5ES26.17E3)') px,py,pz,energy,ms
  case('MAXIMUM')
   read(*,*) boostflag
   boost=boostflag/=0
   call maxscatcen(px,py,pz,energy,ms)
   write(*,'(5ES26.17E3)') px,py,pz,energy,ms
  case('RCONFIG')
   read(*,*) knf,q0,lps,emax,scalefacm,boostflag
   boost=boostflag/=0
   call insudaint(emax)
  case('RTABLE')
   do i=1,1000
    write(*,'(5ES26.17E3)') sudaqq(i,1),sudaqq(i,2),sudaqg(i,2),sudagg(i,2),sudagc(i,2)
   enddo
  case('RSPLIT','RSUDA','RRED','RFAST')
   read(*,*) channel,x,y
   if(trim(op)=='RSPLIT') v=getinspliti(x,channel)
   if(trim(op)=='RSUDA') v=getinsudakov(x,y,channel)
   if(trim(op)=='RRED') v=getinsudared(x,channel)
   if(trim(op)=='RFAST') v=getinsudafast(x,y,channel)
   write(*,'(ES26.17E3)') v
  case('FCONFIG')
   read(*,*) knf,q0,lps,emax,scalefacm,boostflag
   boost=boostflag/=0
   call splitfncint(emax)
  case('FDUMP')
   read(*,'(A)') dumpfile
   open(unit=89,file=trim(dumpfile),access='stream',form='unformatted',status='replace')
   write(89) qval,zmval
   do i=1,1000
    do j=1,1000
     write(89) splitiqqv(i,j),splitiggv(i,j),splitiqgv(i,j)
    enddo
   enddo
   close(89)
  case('FSPLIT')
   read(*,*) channel,qa1,qb1,za1,eb1,angord
   v=getspliti(qa1,qb1,za1,eb1,channel)
   write(*,'(ES26.17E3)') v
  case('FSUDA')
   read(*,*) channel,high,qa1,qb1,za1,eb1,angord
   v=getsudakov(high,qa1,qb1,za1,eb1,channel,0.d0,.false.)
   write(*,'(ES26.17E3)') v
  case('PCONFIG')
   read(*,*) knf,q0,lps,emax,scalefacm,boostflag
   boost=boostflag/=0
   call insudaint(emax)
   call pdfint(emax)
  case('PTABLE')
   do i=1,1000
    write(*,'(5ES26.17E3)') qinqx(1,i),qinqx(2,i),ginqx(2,i),qingx(2,i),gingx(2,i)
   enddo
  case('PDF','PEXACT','PINT')
   read(*,*) channel,x,y
   if(trim(op)=='PDF')v=getpdf(x,y,channel)
   if(trim(op)=='PEXACT')v=getpdfxintexact(x,channel)
   if(trim(op)=='PINT')v=getpdfxint(x,channel)
   write(*,'(ES26.17E3)')v
  case('XCONFIG')
   read(*,*) knf,q0,lps,lqcd,emax,scalefacm,boostflag
   boost=boostflag/=0
   call insudaint(emax)
   call pdfint(emax)
   call xsecint(emax)
  case('XDUMP')
   read(*,'(A)') dumpfile
   open(unit=89,file=trim(dumpfile),access='stream',form='unformatted',status='replace')
   write(89) intq1(1:1000,101),intq1(1001,1:100)
   do i=1,1000
    do j=1,100
     write(89) intq1(i,j),intq2(i,j),intg1(i,j),intg2(i,j)
    enddo
   enddo
   close(89)
  case('XINT')
   read(*,*) channel,x,y
   v=getxsecint(x,y,channel)
   write(*,'(ES26.17E3)')v
  case('NEWMASS')
   read(*,*)qnew2,qold2,previousmass,p(1,5),p(1,4),k(1,2),initial_flag,energyfraction,zdec,qflag
   qqbardec=qflag/=0
   mv(1,4)=0
   read(*,*)count
   if(count<0.or.count>100000)stop 5
   if(count>0)read(*,*)tape(1:count)
   cursor=0
   v=getnewmass(1,qnew2,qold2,previousmass,initial_flag/=0,energyfraction,zdec,qqbardec)
   write(*,'(2ES26.17E3,2I8)')v,zdec,merge(1,0,qqbardec),cursor
  case('NORAD')
   read(*,*)p(1,:),k(1,2),x,y,z,t
   v=getpnorad1(1,x,y,z,t)
   write(*,'(ES26.17E3)')v
  case('XSEC')
   read(*,*) px,py,pz,energy,pmass,lower,incoming,outgoing,x,y,z,t,rate_mode
   v=getsscat(energy,px,py,pz,pmass,lower,incoming,outgoing,x,y,z,t,rate_mode)
   write(*,'(ES26.17E3)')v
  case('TIME')
   read(*,*)p(1,1:5),k(1,2),mv(1,1:4),start,dt,ltime,xsecscalefac,elastic_flag
   read(*,*)count
   if(count<0.or.count>100000)stop 5
   if(count>0)read(*,*)tape(1:count)
   cursor=0
   errl=1
   collision=getdeltat(1,start,dt,elapsed,elastic_flag/=0)
   write(*,'(I2,1X,ES26.17E3,1X,I8)')merge(1,0,collision),elapsed,cursor
  case('REJECT','SPLIT')
   if(trim(op)=='SPLIT')then
    read(*,*)n,mother,ftfac,colmax
    k=0
    p=0
    vertices=0
    mv=0
    starttime=0
    zd=0
    thetad=0
    qqbard=.false.
    aoreq=.false.
    trip=0
    anti=0
   else
    read(*,*)n,mother
   endif
   if(n>23000.or.n<1)stop 5
   mstu=0
   mstu(4)=23000
   paru(1)=3.141592653589793d0
   discard=.false.
   do i=1,n
    if(trim(op)=='SPLIT')then
     read(*,*)k(i,:),p(i,:),vertices(i,1:4),mv(i,:),starttime(i),zd(i),thetad(i),qflag,aflag,trip(i),anti(i)
    else
     read(*,*)k(i,:),p(i,:),vertices(i,1:4),mv(i,:),starttime(i),zd(i),thetad(i),qflag,aflag
    endif
    qqbard(i)=qflag/=0
    aoreq(i)=aflag/=0
   enddo
   read(*,*)count
   if(count<0.or.count>100000)stop 5
   if(count>0)read(*,*)tape(1:count)
   cursor=0
   if(trim(op)=='SPLIT')then
    call makesplitting(mother)
    write(*,'(4I8)')merge(1,0,discard),cursor,n,colmax
   else
    call rejectsplitting(k(mother,4),mother)
    write(*,'(2I8)')merge(1,0,discard),cursor
   endif
   do i=1,n
    if(trim(op)=='SPLIT')then
     write(*,'(5I8,17ES26.17E3,4I8)')k(i,:),p(i,:),vertices(i,1:4),mv(i,:),starttime(i),zd(i),thetad(i), &
      merge(1,0,qqbard(i)),merge(1,0,aoreq(i)),trip(i),anti(i)
    else
    write(*,'(5I8,17ES26.17E3,2I8)')k(i,:),p(i,:),vertices(i,1:4),mv(i,:),starttime(i),zd(i),thetad(i), &
     merge(1,0,qqbard(i)),merge(1,0,aoreq(i))
    endif
   enddo
  case('BCONFIG')
   read(*,*) knf,q0,lps,ptfac,angord
  case('MASS')
   read(*,*) channel,x,y,angle,energy,maxmass,initial_flag
   read(*,*) count
   if(count<0.or.count>100000)stop 5
   if(count>0)read(*,*)tape(1:count)
   cursor=0
   zdec=-1.d0
   qqbardec=.false.
   v=getmass(x,y,angle,energy,channel,maxmass,initial_flag/=0,zdec,qqbardec)
   write(*,'(2ES26.17E3,1X,I0,1X,I0)')v,zdec,merge(1,0,qqbardec),cursor
  case('FRACTION')
   read(*,*) channel,x,y,z
   read(*,*) count
   if(count<0.or.count>100000)stop 5
   if(count>0)read(*,*)tape(1:count)
   cursor=0
   v=generatez(x,y,z,channel)
   write(*,'(ES26.17E3,1X,I0)')v,cursor
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
 double precision x,v,medderiv,getinspliti,getspliti,alphas,pqq,pgg,pqg,pgq,getinsudafast
 double precision qlow,mdx,scatprimfunc
 common /XSECV/ qlow,mdx
 double precision xmax,z
 common /PDFINTV/ xmax,z
 double precision qquad,fm
 common /INTSPLITF/ qquad,fm
 double precision Q0,LPS,LQCD,LTIME,SCALEFACM,RECSOFTCUT,RECHARDCUT
 integer ANGORD,NF,KINMODE,recmode
 logical SCATRECOIL,ALLHAD,compress,mpifsr,aofirstsplit
 common /PARAM/ Q0,LPS,LQCD,LTIME,SCALEFACM,RECSOFTCUT,RECHARDCUT, &
 ANGORD,aofirstsplit,SCATRECOIL,ALLHAD,compress,mpifsr,NF,KINMODE,recmode
 double precision pi
 parameter(pi=3.141592653589793d0)
 double precision qa,za2,eb,t
 logical instate
 character(2) typ
 common /SUDAINT/ qa,za2,eb,t,instate,typ
 integer w
 if(w==1) then
  if(instate) then
   v=2*getinspliti(x,typ)/x
  else
   v=2*getspliti(qa,x,za2,eb,typ)/x
  endif
 else if(w==2) then
  v=(1+fm)*alphas(x*(1-x)*qquad,lps)*pqq(x)/(2*pi)
 else if(w==3) then
  v=(1+fm)*alphas(x*(1-x)*qquad,lps)*pgg(x)/(2*pi)
 else if(w==4) then
  v=(1+fm)*alphas(x*(1-x)*qquad,lps)*pqg(x)/(2*pi)
 else if(w==5) then
  v=exp(-x)/x
 else if(w==6) then
  v=2*getinspliti(x,typ)/x
 else if(w==7) then
  v=2*getinsudafast(x,xmax,'QQ')*alphas((1-z)*x**2,lps)*pqq(z)/(2*pi*x)
 else if(w==8) then
  v=2*getinsudafast(x,xmax,'GC')*alphas((1-z)*x**2,lps)*pgq(z)/(2*pi*x)
 else if(w==9) then
  v=2*getinsudafast(x,xmax,'QQ')*alphas((1-z)*x**2,lps)*pqg(z)/(2*pi*x)
 else if(w==10) then
  v=2*getinsudafast(x,xmax,'GC')*alphas((1-z)*x**2,lps)*2*pgg(z)/(2*pi*x)
 else if(w==11) then
  v=3*getinspliti(scalefacm*sqrt(x),'GQ')*scatprimfunc(x,mdx)/(2*x)
 else if(w==12) then
  v=2*getinspliti(scalefacm*sqrt(x),'QG')*scatprimfunc(x,mdx)/(3*x)
 else if(w==13) then
  v=getinsudafast(qlow,scalefacm*sqrt(x),'GC')*3*2*pi*alphas(x+mdx**2,lps)**2/(2*(x+mdx**2)**2)
 else if(w==14) then
  v=getinsudafast(qlow,scalefacm*sqrt(x),'QQ')*2*2*pi*alphas(x+mdx**2,lps)**2/(3*(x+mdx**2)**2)
 else if(w==21) then
  v=2*getinsudafast(x,xmax,'QQ')*getinspliti(x,'QQ')/x
 else if(w==22) then
  v=2*getinsudafast(x,xmax,'GC')*getinspliti(x,'GQ')/x
 else if(w==23) then
  v=2*getinsudafast(x,xmax,'QQ')*getinspliti(x,'QG')/x
 else if(w==24) then
  v=2*getinsudafast(x,xmax,'GC')*2*getinspliti(x,'GG')/x
 else if(w==101) then
  v=medderiv(x,w-100)
 else
  stop 4
 endif
end function

function pyr(dummy) result(v)
 implicit none
 integer dummy,count,cursor
 double precision tape(100000),v
 common /REPLAY/ tape,count,cursor
 if(cursor>=count) stop 6
 cursor=cursor+1
 v=tape(cursor)
 if(v<=0.or.v>=1) stop 7
end function
