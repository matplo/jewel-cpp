! Harness dispatch for Pythia6 PYP(i,17), copied from the pinned PYP expression.
! The string routine only requests true rapidity. Other requests fail explicitly.
double precision function pyp(i,j)
 implicit none
 integer i,j,n,npad,k(23000,5)
 double precision p(23000,5),v(23000,5),pr,pmr
 common /PYJETS/ n,npad,k,p,v
 if(j/=17.or.i<1.or.i>n)stop 81
 pmr=p(i,5)
 pr=max(1d-20,pmr**2+p(i,1)**2+p(i,2)**2)
 pyp=sign(log(min((sqrt(pr+p(i,3)**2)+abs(p(i,3)))/sqrt(pr),1d20)),p(i,3))
end function

double precision function pyr(dummy)
 implicit none
 integer dummy,count,cursor
 double precision tape(100000)
 common /STRINGTAPE/ tape,count,cursor
 cursor=cursor+1
 if(cursor>count)stop 82
 pyr=tape(cursor)
 if(pyr<=0.or.pyr>=1)stop 83
end function
