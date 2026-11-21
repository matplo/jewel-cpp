"""Compile unchanged overlap expressions extracted from both generator sources."""
import argparse, hashlib, json, math, subprocess, tempfile
from pathlib import Path
p=argparse.ArgumentParser()
p.add_argument('fortran_source',type=Path);p.add_argument('cpp_source',type=Path);p.add_argument('output',type=Path)
a=p.parse_args(); f=a.fortran_source.read_text(); c=a.cpp_source.read_text()
def block(s,start,end):
 return s[s.index(start):s.index(end,s.index(start))+len(end)]
finit=block(f,'        P83A=(1D0-PARP(83))**2','        CQ2R=2D0/(1D0+PARP(84)**2)')
fexpr=block(f,'            OV=(P83A*EXP(-MIN(50D0,B**2))+','     &      P83C*CQ2I*EXP(-MIN(50D0,B**2*CQ2I)))/PARU(2)')
cinit=block(c,'    fracA        = pow2(1. - coreFraction);','    radius2C     = pow2(coreRadius);')
cexpr=block(c,'          overlapNow = normPi * ( fracA * exp( -min(EXPMAX, b*b))','            + fracC * exp( -min(EXPMAX, b*b / radius2C)) / radius2C );')
fcode='''      PROGRAM PROBE
      IMPLICIT NONE
      DOUBLE PRECISION PARP(200),PARU(200),B,OV
      DOUBLE PRECISION P83A,P83B,P83C,CQ2I,CQ2R
      PARU(2)=2D0*ACOS(-1D0)
   10 READ(*,*,END=20) PARP(83),PARP(84),B
'''+finit+'\n'+fexpr+'''
      WRITE(*,'(ES25.17)') OV
      GOTO 10
   20 END
'''
ccode='''#include <algorithm>
#include <cmath>
#include <iostream>
#include <iomanip>
using namespace std;
double pow2(double x){return x*x;}
int main(){double coreFraction,coreRadius,b;const double normPi=1/(2*acos(-1.)),EXPMAX=50.;
while(cin>>coreFraction>>coreRadius>>b){double fracA,fracB,fracC,radius2B,radius2C,overlapNow;
'''+cinit+'\n'+cexpr+'\ncout<<setprecision(17)<<overlapNow<<"\\n";}}\n'
cases=[(fraction,radius,b/20) for fraction in [0,.2,.5,.8,1] for radius in [.1,.4,.7,1] for b in range(201)]
payload=''.join(' '.join(map(str,row))+'\n' for row in cases)
with tempfile.TemporaryDirectory() as td:
 root=Path(td);(root/'probe.f').write_text(fcode);(root/'probe.cpp').write_text(ccode)
 subprocess.run(['/opt/homebrew/bin/gfortran','-O2',str(root/'probe.f'),'-o',str(root/'f')],check=True)
 subprocess.run(['c++','-O2',str(root/'probe.cpp'),'-o',str(root/'c')],check=True)
 values=[[float(v) for v in subprocess.run([str(root/exe)],input=payload,text=True,capture_output=True,check=True).stdout.split()] for exe in ['f','c']]
 assert all(len(v)==len(cases) for v in values)
 failures=[{'case':q,'fortran':x,'cpp':y} for q,x,y in zip(cases,*values) if not math.isfinite(x) or not math.isfinite(y) or not math.isclose(x,y,rel_tol=2e-13,abs_tol=0)]
 report={'scope':'extracted MPI overlap expressions only; not impact-parameter sampling or complete MPI parity','cases':len(cases),'relative_tolerance':2e-13,'absolute_tolerance':0,'max_absolute_error':max(abs(x-y) for x,y in zip(*values)),'max_relative_error':max(abs(x-y)/abs(x) for x,y in zip(*values)), 'failures':failures,'source_sha256':{str(src):hashlib.sha256(src.read_bytes()).hexdigest() for src in [a.fortran_source,a.cpp_source]},'extracted_sources':{'fortran':fcode,'cpp':ccode},'input_sha256':hashlib.sha256(payload.encode()).hexdigest()}
 a.output.write_text(json.dumps(report,indent=2)+'\n')
 print({k:v for k,v in report.items() if k not in ['extracted_sources','source_sha256','failures']});assert not failures,failures[:3]
