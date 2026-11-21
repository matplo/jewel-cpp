      SUBROUTINE JG_LENGTHS(X0,Y0,B,R,TMAX,P4,STEP,LG,LH,LHT,IT3)
      USE, INTRINSIC :: IEEE_ARITHMETIC, ONLY: IEEE_IS_FINITE
      IMPLICIT NONE
      DOUBLE PRECISION X0,Y0,B,R,TMAX,P4(4),STEP,LG,LH,LHT,IT3
      DOUBLE PRECISION VX,VY,VZ,SPEED,VT,NX,NY,X,DOT,DISC,DIST
      DOUBLE PRECISION TEND,DT,T,TEMP,GETTEMP,RATIO
      INTEGER I,J,NSTEPS
      LG=0.D0
      LH=0.D0
      LHT=0.D0
      IT3=0.D0
      IF(R.LE.0.D0) RETURN
      DO J=1,4
        IF(.NOT.IEEE_IS_FINITE(P4(J)))
     &       CALL JG_FAIL('non-finite shower-start momentum')
      ENDDO
      IF(P4(4).LE.0.D0) CALL JG_FAIL('nonpositive shower-start energy')
      VX=P4(1)/P4(4)
      VY=P4(2)/P4(4)
      VZ=P4(3)/P4(4)
      SPEED=SQRT(VX**2+VY**2+VZ**2)
      VT=SQRT(VX**2+VY**2)
      IF(SPEED.GT.1.D0+1.D-8)
     &     CALL JG_FAIL('spacelike shower-start momentum')
      IF(SPEED.EQ.0.D0) RETURN
      TEND=MAX(0.D0,TMAX)
C--The NTHICK support is the intersection of two discs of radius R.
      IF(VT.GT.0.D0)THEN
        NX=VX/VT
        NY=VY/VT
        LG=HUGE(LG)
        DO J=1,2
          X=X0+(2*J-3)*B/2.D0
          DOT=X*NX+Y0*NY
          DISC=DOT**2+R**2-X**2-Y0**2
          IF(X**2+Y0**2.GT.R**2*(1.D0+1.D-10))
     &         CALL JG_FAIL('production point outside NTHICK support')
          DIST=MAX(0.D0,-DOT+SQRT(MAX(0.D0,DISC)))
          LG=MIN(LG,DIST)
        ENDDO
        TEND=MIN(TEND,LG/VT)
      ENDIF
      IF(TEND.LE.0.D0) RETURN
C--STEP bounds coordinate time (fm/c) and hence spatial step (fm).
C--Never stop at the first zero: the temperature initially rises.
      RATIO=TEND/STEP
      IF(.NOT.IEEE_IS_FINITE(RATIO))
     &     CALL JG_FAIL('geometry step requires too many intervals')
      IF(RATIO.GT.10000000.D0)
     &     CALL JG_FAIL('geometry step exceeds 10000000 intervals')
      NSTEPS=MAX(1,CEILING(RATIO))
      DT=TEND/DBLE(NSTEPS)
      DO I=1,NSTEPS
        T=(DBLE(I)-0.5D0)*DT
        TEMP=GETTEMP(X0+VX*T,Y0+VY*T,VZ*T,T)
        IF(.NOT.IEEE_IS_FINITE(TEMP))
     &       CALL JG_FAIL('medium returned non-finite temperature')
        IF(TEMP.GT.0.D0)THEN
          LH=LH+SPEED*DT
          LHT=LHT+VT*DT
          IT3=IT3+TEMP**3*SPEED*DT
        ENDIF
      ENDDO
      END

      SUBROUTINE JG_FAIL(MESSAGE)
      IMPLICIT NONE
      CHARACTER*(*) MESSAGE
      WRITE(*,*) MESSAGE
      STOP 1
      END
