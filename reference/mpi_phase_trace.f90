subroutine coupling_probe
implicit none
integer mint(400),mstp(200),msti(200),phase
common /MPIAUDIT/ phase
real(8) vint(400),parp(200),pari(200),sigt(0:6,0:6,0:5)
common /PYINT1/ mint,vint
common /PYPARS/ mstp,parp,msti,pari
common /PYINT7/ sigt
write(*,*) 'MPI_NORMALIZATION',vint(1),vint(145),sigt(0,0,:), &
 mstp(81:84),parp(82:84),parp(89:90)
phase=1
end subroutine

subroutine mpi_audit_begin
implicit none
integer phase
common /MPIAUDIT/ phase
phase=0
end subroutine
subroutine mpi_coupling_trace(q2,q2sf,pt2,alphas)
implicit none
real(8) q2,q2sf,pt2,alphas
integer mstu(200),mstj(200),mstp(200),msti(200),mint(400),stage
real(8) paru(200),parj(200),parp(200),pari(200),vint(400)
integer phase
common /MPIAUDIT/ phase
integer,save::counts(0:1,0:10)=0
common /PYDAT1/ mstu,paru,mstj,parj
common /PYPARS/ mstp,parp,msti,pari
common /PYINT1/ mint,vint
stage=min(10,max(0,mint(35)))
if(counts(phase,stage)>=20)return
counts(phase,stage)=counts(phase,stage)+1
write(*,*) 'MPI_TRACE',phase,stage,mint(31),q2,q2sf,pt2,alphas, &
 mstu(111:115),paru(112),mstp(81),mstp(82),mstp(32),mstp(33), &
 parp(34),parp(82),parp(89),parp(90),vint(1)
end subroutine
