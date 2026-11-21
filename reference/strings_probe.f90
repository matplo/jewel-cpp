program probe
 implicit none
 integer n,npad,k(23000,5),trip(23000),anti(23000),colmax,hpmcfid,logfid
 double precision p(23000,5),v(23000,5),mv(23000,5),zd(23000),theta(23000)
 logical pair(23000),ao(23000)
 common /PYJETS/ n,npad,k,p,v
 common /COLOUR/ trip,anti,colmax
 common /TIME/ mv
 common /ANGOR/ zd,theta,pair,ao
 common /HEPMCID/ hpmcfid,logfid
 double precision q0,lps,lqcd,ltime,scalefacm,recsoftcut,rechardcut,wdisc
 integer angord,nf,kinmode,recmode,ndisc,nstrange,ngood,errcount
 logical aofirstsplit,scatrecoil,allhad,compress,mpifsr,discard
 common /PARAM/ q0,lps,lqcd,ltime,scalefacm,recsoftcut,rechardcut, &
  angord,aofirstsplit,scatrecoil,allhad,compress,mpifsr,nf,kinmode,recmode
 common /DISC/ ndisc,nstrange,ngood,errcount,wdisc,discard
 integer i,ios,flag,count,cursor
 double precision tape(100000),etamax
 common /STRINGTAPE/ tape,count,cursor
 common /RAPMAX/ etamax
 character(32) arg
 logical minimum
 call get_command_argument(1,arg)
 minimum=trim(arg)=='--minimum'
 logfid=0
 do
  read(*,*,iostat=ios)n,flag
  if(ios/=0)exit
  if(n<0.or.n>22990)stop 1
  if(minimum)then
   read(*,*)etamax,count
   if(count<0.or.count>100000)stop 80
   if(count>0)read(*,*)tape(1:count)
   cursor=0
  endif
  k=0;p=0;v=0;mv=0;trip=0;anti=0;zd=0;theta=0;pair=.false.;ao=.false.
  compress=flag/=0;discard=.false.
  do i=1,n
   read(*,*)k(i,:),p(i,:),v(i,:),mv(i,:),trip(i),anti(i)
  enddo
  if(minimum)then
   call makestrings_minl
   write(*,*)n,merge(1,0,discard),cursor
  else
   call makestrings_vac
   write(*,*)n,merge(1,0,discard)
  endif
  do i=1,n
   write(*,*)k(i,:),p(i,:),v(i,:),mv(i,:),trip(i),anti(i)
  enddo
 enddo
end program
