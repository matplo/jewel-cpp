subroutine mpi_coupling_trace(q2,q2sf,pt2,alphas)
implicit none
real(8) q2,q2sf,pt2,alphas
integer mstu(200),mstj(200),mstp(200),msti(200),mint(400),stage
real(8) paru(200),parj(200),parp(200),pari(200),vint(400)
integer,save::counts(0:10)=0
common /PYDAT1/ mstu,paru,mstj,parj
common /PYPARS/ mstp,parp,msti,pari
common /PYINT1/ mint,vint
stage=min(10,max(0,mint(35)))
if(counts(stage)>=20)return
counts(stage)=counts(stage)+1
write(*,*) 'MPI_TRACE',stage,mint(31),q2,q2sf,pt2,alphas, &
 mstu(111:115),paru(112),mstp(81),mstp(82),mstp(32),mstp(33), &
 parp(34),parp(82),parp(89),parp(90),vint(1)
end subroutine
