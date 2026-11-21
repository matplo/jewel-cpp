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

logical function compressevent(line)
 implicit none
 integer line
 stop 71 ! Compression is disabled in scheduler fixtures; never silently stub it.
end function
subroutine pevrec(mode,flag)
 implicit none
 integer mode
 logical flag
 stop 72 ! Invalid scheduler time diagnostics must fail the reference fixture.
end subroutine
