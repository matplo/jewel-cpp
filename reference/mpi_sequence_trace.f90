subroutine seqstate(xt,nstr,kstr,done)
implicit none
integer nstr,kstr(500,2),done,n,npad,k(23000,5),mint(400),i,j
real(8) xt,p(23000,5),v(23000,5),vint(400)
common /PYJETS/ n,npad,k,p,v
common /PYINT1/ mint,vint
write(*,*) 'SEQ_STATE',xt,vint(143),vint(144),vint(151),vint(152), &
 mint(31),mint(351),vint(351),vint(356),n,nstr,done
do i=1,n
 write(*,*) 'SEQ_PARTICLE',k(i,2),p(i,1:4)
enddo
do i=1,nstr
 write(*,*) 'SEQ_STRING',kstr(i,:)
enddo
end subroutine
block data seqinitial
implicit none
integer active
common /SEQACTIVE/ active
data active /0/
end block data
