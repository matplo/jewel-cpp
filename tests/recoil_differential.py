import hashlib,itertools,json,math,random,subprocess,sys
from pathlib import Path
cpp,reference,output=sys.argv[1:]
rng=random.Random(26006261);cases=[]
# Same-mass on-shell incoming/outgoing centres give spacelike transfer records.
for index in range(160):
 mass=10**rng.uniform(-2,1)
 p=[rng.uniform(-4,4) for _ in range(3)];q=[rng.uniform(-4,4) for _ in range(3)]
 p+=[math.sqrt(sum(v*v for v in p)+mass*mass),mass]
 q+=[math.sqrt(sum(v*v for v in q)+mass*mass)]
 for mode,soft in itertools.product(range(4),range(2)):
  cases.append([mode,1,0,soft,0,1,3,21 if index%2 else 2,*p,*q])
# Exhaust storage gates, including an existing populated tail and zero capacity.
for mode,write,subtract,soft,rejected,used,capacity in itertools.product(range(4),range(2),range(2),range(2),range(2),range(3),range(3)):
 if used>capacity:continue
 cases.append([mode,write,subtract,soft,rejected,used,capacity,21,0,0,0,1,1,2,0,0,math.sqrt(5)])
payload=''.join(' '.join(format(v,'.17g') for v in c)+'\n' for c in cases)
def run(exe):
 result=subprocess.run([exe],input=payload,text=True,capture_output=True,check=True)
 return [list(map(float,line.split())) for line in result.stdout.splitlines()]
a,b=run(cpp),run(reference);assert len(a)==len(b)==len(cases)
failures=[];maximum=0;scalars=0;negative_transfers=0;untouched_soft=0
for i,(c,x,y) in enumerate(zip(cases,a,b)):
 mode,write,subtract,soft,rejected,used,capacity,flavour=c[:8]
 assert len(x)==len(y)==1+11*capacity
 for col,(v,w) in enumerate(zip(x,y)):
  scalars+=1;maximum=max(maximum,abs(v-w))
  discrete=col==0 or (col-1)%11==0
  if not math.isfinite(v) or (v!=w if discrete else not math.isclose(v,w,rel_tol=2e-12,abs_tol=2e-20 if col>0 and (col-1)%11>=6 else 2e-13)):
   failures.append(dict(case=i,column=col,cpp=v,fortran=w))
 active=(write or subtract) and not rejected and used<capacity
 assert x[0]==used+int(active)
 if not active:continue
 record=x[1+11*used:1+11*(used+1)]
 if soft and mode in (0,1):
  assert record==[-99,1,2,3,4,5,6,7,8,9,10];untouched_soft+=1;continue
 assert record[0]==flavour and record[9]==1e-6
 transfer=mode==2 or (mode==3 and soft)
 assert (record[10]<0) if transfer else (record[10]>=0)
 # Momentum projection follows the intended incoming/outgoing/transfer direction.
 direction=c[13:17] if mode==1 else ([c[13+j]-c[8+j] for j in range(4)] if transfer else c[8:12])
 for j in range(3):assert math.isclose(record[6+j],direction[j]*1e-6/(abs(direction[3]) if transfer else direction[3]),rel_tol=2e-12,abs_tol=1e-20)
 negative_transfers+=bool(transfer and record[4]<0)
report=dict(scope='DOKINEMATICS recoil-storage section only (lines 6261-6376)',cases=len(cases),scalars=scalars,max_absolute_error=maximum,relative_tolerance=2e-12,absolute_tolerance=2e-13,dummy_absolute_tolerance=2e-20,negative_energy_transfer_cases=negative_transfers,untouched_soft_slots=untouched_soft,failures=failures,input_sha256=hashlib.sha256(payload.encode()).hexdigest(),source=json.loads((Path(__file__).parents[1]/'reference/recoil-source.json').read_text()),executables={Path(e).name:hashlib.sha256(Path(e).read_bytes()).hexdigest() for e in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k not in ['source','executables']},indent=2))
if failures:raise SystemExit('Recoil parity failed')
