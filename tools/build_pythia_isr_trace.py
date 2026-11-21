"""Build an isolated source-observation shower; never modify installed Pythia."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('source',type=Path,help='unmodified Pythia 8.317 source root')
parser.add_argument('prefix',type=Path,help='installed Pythia 8.317 prefix')
parser.add_argument('output',type=Path,help='new diagnostic build directory')
parser.add_argument('--cxx',default='c++')
args=parser.parse_args()
expected={'src/SimpleSpaceShower.cc':'953dd8b8fee0a75b9a297f9c6e854586aed42ed66576e6a3ddb61b568b4d696f',
 'include/Pythia8/SimpleSpaceShower.h':'b145926773ead27097376a45b4881dceb86f74e4f3104b978b583ce89822c5d7'}
inputs={}
for name,digest in expected.items():
    path=args.source/name
    assert hashlib.sha256(path.read_bytes()).hexdigest()==digest, f'unverified source: {path}'
    inputs[name]=path.read_text()
assert hashlib.sha256((args.prefix/'include/Pythia8/SimpleSpaceShower.h').read_bytes()).hexdigest()==expected['include/Pythia8/SimpleSpaceShower.h']
args.output.mkdir(parents=True,exist_ok=False)
(args.output/'Pythia8').mkdir()
def rename(text):
    return text.replace('SimpleSpaceShower','TraceSpaceShower').replace('SpaceDipoleEnd','TraceSpaceDipoleEnd')
(args.output/'Pythia8/TraceSpaceShower.h').write_text(rename(inputs['include/Pythia8/SimpleSpaceShower.h']))
s=rename(inputs['src/SimpleSpaceShower.cc'])
s=s.replace('#include "Pythia8/TraceSpaceShower.h"','#include "Pythia8/TraceSpaceShower.h"\n#include "pythia_isr_trace.hpp"')
s=s.replace('bool TraceSpaceShower::branch( Event& event) {','bool TraceSpaceShower::branch( Event& event) {\n  RotBstMatrix recordedMatrix; bool recordedMatrixValid=false; double recordedPhi=0.; RotBstMatrix recordedFromRest; Vec4 recordedSisterRest; RotBstMatrix recordedRestBoost; double recordedTheta=0.;')
s=s.replace('    Mtot.rotbst(MfromRest);','    Mtot.rotbst(MfromRest);\n    recordedMatrix=Mtot;recordedMatrixValid=true;recordedPhi=phi;recordedFromRest=MfromRest;recordedSisterRest=pSister;')
s=s.replace('    MfromRest.bst( -betaX, 0., -betaZ);','    MfromRest.bst( -betaX, 0., -betaZ);\n    recordedRestBoost=MfromRest;')
s=s.replace('    MfromRest.rot(-theta, phi);','    recordedTheta=theta;\n    MfromRest.rot(-theta, phi);')
start=s.index('bool TraceSpaceShower::branch(')
pos=s.index('  // Done without any errors.\n  return true;',start)
s=s[:pos]+'  if(recordedMatrixValid) record_isr(iSysSel,iMother,iNewRec,iSister,recordedMatrix,event,Q2,z,m2II,x1,x2,x1New,x2New,pT2corr,recordedPhi,recordedFromRest,recordedSisterRest,recordedRestBoost,recordedTheta);\n'+s[pos:]
(args.output/'TraceSpaceShower.cc').write_text(s)
reference=Path(__file__).resolve().parents[1]/'reference'
command=[args.cxx,'-O3','-std=c++17',str(args.output/'TraceSpaceShower.cc'),str(reference/'pythia_isr_trace.cpp'),
 '-I'+str(args.output),'-I'+str(reference),'-I'+str(args.prefix/'include'),'-L'+str(args.prefix/'lib'),
 '-Wl,-rpath,'+str(args.prefix/'lib'),'-lpythia8','-o',str(args.output/'probe')]
subprocess.run(command,check=True)
manifest=dict(source_hashes=expected,command=command,
 reference_hashes={name:hashlib.sha256((reference/name).read_bytes()).hexdigest() for name in ['pythia_isr_trace.cpp','pythia_isr_trace.hpp']},
 generated_source_sha256=hashlib.sha256((args.output/'TraceSpaceShower.cc').read_bytes()).hexdigest(),
 executable_sha256=hashlib.sha256((args.output/'probe').read_bytes()).hexdigest())
(args.output/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(args.output/'probe')
