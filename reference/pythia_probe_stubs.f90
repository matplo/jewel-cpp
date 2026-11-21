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
