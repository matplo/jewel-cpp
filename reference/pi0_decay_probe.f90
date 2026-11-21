program probe
implicit none
integer n,npad,k(23000,5),mstu(200),mstj(200),mdcy(500,3),mdme(8000,2),kfdp(8000,5),pycomp,mode,i,npion,nfinal
real(8) mom(23000,5),v(23000,5),paru(200),parj(200),brat(8000),energy
common /PYJETS/ n,npad,k,mom,v
common /PYDAT1/ mstu,paru,mstj,parj
common /PYDAT3/ mdcy,mdme,brat,kfdp
mstj(22)=2
 do mode=0,1
  mdcy(pycomp(111),1)=mode
  call py1ent(1,111,1d0,0d0,0d0)
  call pyexec
  npion=0;nfinal=0;energy=0d0
  do i=1,n
   if(k(i,1)>0.and.k(i,1)<11)then
    nfinal=nfinal+1;energy=energy+mom(i,4)
    if(k(i,2)==111)npion=npion+1
   endif
  enddo
  write(*,*) 'PI0_RESULT',mode,npion,nfinal,energy
 enddo
end program
