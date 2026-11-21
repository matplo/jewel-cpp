! PEVREC is extracted unchanged, including its V=MV state mutation.
! Only textual PYLIST output is replaced; diagnostics remain visible on stderr.
subroutine pylist(mode)
 implicit none
 integer mode
 if(mode/=2.and.mode/=3)stop 74
 write(0,*) 'REFERENCE_RECORD_LISTING ',mode
end subroutine
