! SPDX-License-Identifier: GPL-2.0-or-later
! Diagnostic scheduler with the MAKECASCADE visit order. MAKEBRANCH and all
! physics routines are unmodified extracted source. Abort at the first
! nonfinite post-branch state, or at an explicit branch-call limit.
subroutine guarded_cascade(limit,reason,calls)
 use, intrinsic :: ieee_arithmetic
 implicit none
 integer limit,reason,calls,n,npad,k(23000,5),nold,i
 double precision p(23000,5),v(23000,5),mv(23000,5),zd(23000),theta(23000),starttime(23000),wdisc
 logical pair(23000),ao(23000),medind(23000),discard,active
 integer ndisc,nstrange,ngood,errcount
 double precision tape(2000000)
 integer count,cursor,trace_status
 character(8) trace_value
 logical trace
 common /REPLAY/ tape,count,cursor
 common /PYJETS/ n,npad,k,p,v
 common /TIME/ mv
 common /ANGOR/ zd,theta,pair,ao
 common /EVOLTIMES/ starttime,medind
 common /DISC/ ndisc,nstrange,ngood,errcount,wdisc,discard
 call get_environment_variable('JEWEL_CASCADE_TRACE',trace_value,status=trace_status)
 trace=trace_status==0.and.trim(trace_value)=='1'
 calls=0;reason=0
10 nold=n;active=.false.
 do i=2,nold
  if(i>n)goto 10
  if(k(i,1)==1.or.k(i,1)==2)then
   active=.true.
   if(calls>=limit)then
    reason=2;return
   endif
   calls=calls+1
   if(trace)write(0,'(A,6I12,9ES26.17E3)')'CASCADE_TRACE ',0,calls,i,n,cursor,merge(1,0,discard), &
    p(i,:),zd(i),starttime(i),mv(i,4),mv(i,5)
   call makebranch(i)
   if(trace)write(0,'(A,6I12)')'CASCADE_TRACE ',1,calls,i,n,cursor,merge(1,0,discard)
   if(.not.all(ieee_is_finite(p(1:n,:))).or..not.all(ieee_is_finite(v(1:n,:))).or. &
      .not.all(ieee_is_finite(mv(1:n,:))).or..not.all(ieee_is_finite(zd(1:n))).or. &
      .not.all(ieee_is_finite(theta(1:n))).or..not.all(ieee_is_finite(starttime(1:n))))then
    reason=1;return
   endif
   if(discard)return
  endif
 enddo
 if(active)goto 10
end subroutine
