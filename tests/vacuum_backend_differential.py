"""Vacuum fields, vertex sampling and numerical tables against frozen medium-vac.f."""
import array,hashlib,json,math,subprocess,sys,tempfile
from pathlib import Path
cpp,reference,output=sys.argv[1:]
groups={};failures=[];hashes=[]
def compare(label,a,b):
    assert len(a)==len(b),(label,len(a),len(b))
    g=groups.setdefault(label,dict(values=0,max_absolute_error=0.,outside_tolerance=0))
    for i,(x,y) in enumerate(zip(a,b)):
        g['values']+=1;g['max_absolute_error']=max(g['max_absolute_error'],abs(x-y))
        if not math.isfinite(x) or not math.isfinite(y) or not math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13):
            g['outside_tolerance']+=1
            if len(failures)<20:failures.append(dict(group=label,index=i,cpp=x,fortran=y))
payload='VACUUM\nBOUNDS\nMAXIMUM\n0\nMAXIMUM\n1\n'
for t in [-1,0,.1,.4,1,100]:
    for z in [-100,-1,0,1,100]:
        for x,y in [(0,0),(3,4),(100,-100)]:payload+=f'POINT\n{x} {y} {z} {t}\n'
for b in [0,3,8]:payload+=f'VERTEX\n{b} 10\n1\n.5\n'
values=[]
for exe in [cpp,reference]:
    run=subprocess.run([exe],input=payload,text=True,capture_output=True,check=True)
    values.append([float(x) for x in run.stdout.split()])
compare('fields-bounds-maximum-vertex',*values)
# Verify the vacuum contract independently of agreement between implementations.
assert values[0][:7]==[0]*7
assert values[0][7:17]==[0,0,0,.5,.5]*2
assert values[0][17:]==[0]*(len(values[0])-17)
with tempfile.TemporaryDirectory() as tmp:
    for energy in [100,1.5*150*math.cosh(2)]:
        tables=[];scalars=[]
        for exe in [cpp,reference]:
            path=Path(tmp)/(Path(exe).name+'.bin')
            query=f'VACUUM\nRCONFIG\n3 1 .2 {energy:.17g} 1 1\nRTABLE\nPCONFIG\n3 1 .2 {energy:.17g} 1 1\nPTABLE\nFCONFIG\n3 1 .2 {energy:.17g} 1 1\nFDUMP\n{path}\n'
            run=subprocess.run([exe],input=query,text=True,capture_output=True,check=True)
            scalars.append([float(x) for x in run.stdout.split()])
            raw=path.read_bytes();table=array.array('d');table.frombytes(raw)
            assert len(table)==3002000
            tables.append(table);hashes.append(dict(executable=Path(exe).name,energy=energy,table_sha256=hashlib.sha256(raw).hexdigest()))
        compare('sudakov-and-pdf-tables',*scalars)
        compare('final-splitting-tables',*tables)
report=dict(scope='Frozen JEWEL vacuum backend and initialized tables, not full generator equivalence',groups=groups,failures=failures,records=hashes,relative_tolerance=2e-12,absolute_tolerance=2e-13,vacuum_source_sha256='c83fda6a0b81ce0dc9f524bbf9a3c68f0225f090107a5cd4d6711edad0b20ea0',executables={Path(p).name:hashlib.sha256(Path(p).read_bytes()).hexdigest() for p in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(groups,indent=2))
if failures:raise SystemExit(str(failures))
