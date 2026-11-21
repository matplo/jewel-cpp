program driver
implicit none
integer mstu(200),mstj(200),ids(4),i,j,l
real(8) paru(200),parj(200),mt(3),a,b,c
common /PYDAT1/ mstu,paru,mstj,parj
ids=[1,3,2103,3103];mt=[.1d0,1d0,4d0]
mstj(11)=5
parj(41)=.5;parj(42)=.6;parj(47)=.67
write(*,*)'SOURCE_DEFAULT',parj(45)
do i=1,4
 do j=1,4
  do l=1,3
   call shape_probe(ids(i),ids(j),mt(l),a,b,c)
   write(*,*)'SHAPE',ids(i),ids(j),mt(l),a,b,c
  enddo
 enddo
enddo
end program
