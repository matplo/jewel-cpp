"""Controlled scheduler fixtures, not Pythia hard-collision events."""
import math,random

def build_cases(*,baseline_only=False,guarded=False,generated=False,case_index=None,seed_offset=0,coherence=None):
 def row(v):return ' '.join(format(x,'.17g') for x in v)+'\n'
 base='CONFIG\n208 3 1 .4 .59 .17 .17 .54 6.199999809265137 0 2 .45 1\nFCONFIG\n3 1 .2 100 1 0\nRCONFIG\n3 1 .2 100 1 0\nXCONFIG\n3 1 .2 .2 100 1 0\nPCONFIG\n3 1 .2 100 1 0\nQCONFIG\n3 .2 .2 0\nBCONFIG\n3 1 .2 1 0\n'
 cases=[];labels=[];capacity=64;active_nf=3;active_boost=0

 def rec(status,pdg,parent,daughters,p,mass,colour,anti):
  return [status,pdg,parent,*daughters,*p,mass,0,0,0,0,0,0,0,.4,8,.4,.3,0,0,colour,anti,.5]
 def append(pdg,first,second,direction,selected,left,cross,length,kinmode=1,history=0,recmode=0,subtract=0,energy=20,mass=4,rotation=0,shorten=False,weight=1,enabled=1,near_edge=False,retry=False,rejection=0,force_radiation=False,operation="BRANCH",status=1,start=.5,birth=.4,scatter_recoils=1):
  if case_index is not None and len(cases)!=case_index:
   cases.append('');labels.append({});return
  if coherence is not None:enabled=int(coherence)
  momentum=math.sqrt(energy*energy-mass*mass);z=.4;ma=3 if mass>=10 else .3;mb=2 if mass>=10 else .5
  if near_edge:
   rest_energy=(mass*mass+mb*mb-ma*ma)/(2*mass)
   rest_momentum=math.sqrt(rest_energy*rest_energy-mb*mb)
   z=(rest_energy-momentum/energy*rest_momentum)/mass+1e-7
  pz=(2*z*energy*energy-mass*mass-mb*mb+ma*ma)/(2*momentum)
  pt=math.sqrt(z*z*energy*energy-pz*pz-mb*mb)
  def rotate(p):
   x,y,z,e=p;theta=rotation;phi=.7 if rotation else 0
   x,z=math.cos(theta)*x+math.sin(theta)*z,-math.sin(theta)*x+math.cos(theta)*z
   return [math.cos(phi)*x-math.sin(phi)*y,math.sin(phi)*x+math.cos(phi)*y,z,e]
  parent=rotate([0,0,momentum,energy])
  records=[rec(0,0,0,[0,0],[0,0,0,0],0,0,0),rec(11,21,0,[0,0],parent,mass,101,102),
   rec(11,pdg,2,[4,5],parent,mass,101,102),rec(1,first,3,[0,0],rotate([-pt,0,momentum-pz,(1-z)*energy]),ma,101 if direction>=0 else 103,104),
   rec(1,second,3,[0,0],rotate([pt,0,pz,z*energy]),mb,104,102)]
  records[2][18]=birth
  for r in records[3:]:
   r[0]=status;r[17]=birth;r[18]=max(.6,.5+left);r[25]=start
  if status==2:
   for r in records[3:]:r[19]=-1;r[9]=0;r[8]=math.sqrt(sum(v*v for v in r[5:8]))
  records[0]=[0]*26;records[0][19]=-1
  if shorten:records[4 if selected==4 else 3][18]=1.2
  seed=260000+len(cases)+seed_offset;rng=random.Random(seed);tape_count=2000000 if operation=="CASCADE" and cross>=5 else 500000 if operation=="CASCADE" and cross else 100000
  tape=[] if generated else [rng.uniform(.000001,.999999) for _ in range(tape_count)]
  if force_radiation and generated:raise ValueError("forced draws require replay mode")
  if force_radiation:tape[:4]=[.999999,.000001,.999999,.9]
  if guarded and operation=='CASCADE':operation='GCASCADE'
  config=f'ECONFIG\n1 1 1 3 {cross}\n{operation}\n'+row([selected,enabled,1,left,.05,.4,length,kinmode,110,weight,history])
  config+=row([int(retry),rejection])
  config+=''.join(row(r) for r in records)+row([1,0,0,1,scatter_recoils,recmode,1,subtract,capacity])
  for _ in range(history):config+=row([-.2,.1,.2,.3,.01,.6,21,0,0,0,1,1,0,0,0,.6])
  config+=('-1\n'+str(seed)+'\n') if generated else str(tape_count)+'\n'+row(tape)
  cases.append(config);labels.append(dict(pdg=pdg,selected=selected,left=left,cross=cross,length=length,seed=seed,kinmode=kinmode,history=history,recmode=recmode,subtract=subtract,energy=energy,mass=mass,rotation=rotation,shorten=shorten,weight=weight,enabled=enabled,near_edge=near_edge,nf=active_nf,boost=active_boost,retry=retry,rejection=rejection,force_radiation=force_radiation,selected_flavour=first if selected==4 else second,operation=operation,status=status,start=start,birth=birth,scatter_recoils=scatter_recoils))
 for pdg,first,second in [(21,21,21),(1,21,1),(-1,21,-1)]:
  for selected in [4,5]:
   for left,cross,enabled in [(0,0,0),(.5,0,0),(.5,1,0),(.5,5,0),(.5,1,1)]:
    append(pdg,first,second,1,selected,left,cross,1,enabled=enabled)
 for status in [1,2]:
  for start,birth in [(-1e6,.4),(-1e6,-.5),(-.2,-.5),(1,.4)]:
   append(21,21,21,1,4,.5,0,1,enabled=0,status=status,start=start,birth=birth)
 for pdg,first,second in [(21,21,21),(1,21,1)]:
  for status in [1,2]:
   append(pdg,first,second,1,4,.5,0,1,enabled=0,status=status,operation="CASCADE",scatter_recoils=0)
 if not baseline_only:
  for pdg,first,second in [(21,21,21),(1,21,1),(-1,21,-1)]:
   for cross,enabled in [(0,0),(1,0),(1,1),(5,1)]:
    append(pdg,first,second,1,4,.5,cross,1,enabled=enabled,energy=100,mass=10,operation="CASCADE",scatter_recoils=0)
  for kinmode in [0,1,2,3]:
   append(21,21,21,1,4,.5,1,1,enabled=0,energy=100,mass=10,rotation=.8,kinmode=kinmode,operation="CASCADE",scatter_recoils=0)
 case_indices=list(range(len(cases)))
 if case_index is not None:
  if case_index<0 or case_index>=len(cases):raise ValueError('case outside matrix')
  case_indices=[case_index];cases=[cases[case_index]];labels=[labels[case_index]]
 return base,cases,labels,case_indices
