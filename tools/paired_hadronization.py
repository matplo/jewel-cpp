"""Build matched pre-fragmentation, PYEXEC and Pythia8 analysis streams."""
import argparse
from collections import Counter
import hashlib
import json
import math
from pathlib import Path
import subprocess
import shutil
from contextlib import ExitStack

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('input',type=Path)
parser.add_argument('pyexec',type=Path)
parser.add_argument('directory',type=Path)
args=parser.parse_args()
args.directory.mkdir(exist_ok=False)
with args.input.open() as f:
    source_header=json.loads(next(f))
assert source_header['type']=='run'

def source_events():
    with args.input.open() as f:
        for line in f:
            event=json.loads(line)
            if event['type']=='event':
                yield event

def total(particles):
    return [math.fsum(p['momentum'][i] for p in particles) for i in range(4)]

def packed(event):
    if event['hadronization_junctions']:
        raise ValueError('junction')
    parts=event['hadronization_input']
    ports=[{},{}]
    for i,p in enumerate(parts,1):
        for side,key in enumerate(['colour','anticolour']):
            tag=p[key]
            if tag:
                if tag in ports[side]:
                    raise ValueError('repeated colour port')
                ports[side][tag]=i
    if ports[0].keys()!=ports[1].keys():
        raise ValueError('open colour line')
    yield [event['attempt'],len(parts)]
    for p in parts:
        yield [p['pdg'],ports[1].get(p['colour'],0),ports[0].get(p['anticolour'],0),
               *p['momentum'],p['mass'],*p['vertex_mm']]

ineligible=set()
source_count=0
excluded=Counter()
with (args.directory/'pyexec-input.dat').open('w') as f:
    for event in source_events():
        source_count+=1
        try:
            rows=list(packed(event))
        except ValueError as e:
            excluded[str(e)]+=1
            ineligible.add(event['attempt'])
            continue
        for row in rows:
            f.write(' '.join(map(str,row))+'\n')
with (args.directory/'pyexec-input.dat').open() as source, (args.directory/'pyexec-output.dat').open('w') as out, (args.directory/'pyexec.log').open('w') as log:
    subprocess.run([str(args.pyexec.resolve())],stdin=source,stdout=out,stderr=log,check=True)
shared_count=0
weight_sum=0.
max_residual=[0.,0.]
stages=['before','pythia6','pythia8']
body_paths=[args.directory/(stage+'.body') for stage in stages]
with ExitStack() as stack:
    dump=stack.enter_context((args.directory/'pyexec-output.dat').open())
    bodies=[stack.enter_context(path.open('w')) for path in body_paths]
    for event in source_events():
        if event['attempt'] in ineligible:
            continue
        tag,number,error,count=next(dump).split()
        assert tag=='EVENT' and int(number)==event['attempt']
        p6=[]
        for _ in range(int(count)):
            pid,*values=next(dump).split()
            p6.append(dict(pdg=int(pid),final=True,momentum=list(map(float,values))))
        if int(error):
            excluded['PYEXEC error']+=1
            continue
        before=event['hadronization_input']
        p8=[p for p in event['hadronized_particles'] if p['final']]
        baseline=total(before)
        residuals=[max(abs(a-b) for a,b in zip(total(parts),baseline)) for parts in [p6,p8]]
        if any(not math.isfinite(v) or v>1e-8*max(1,baseline[3]) for v in residuals):
            excluded['momentum closure']+=1
            continue
        if any(abs(p['pdg']) in range(1,7) or p['pdg']==21 for p in p6):
            excluded['unfragmented quark or gluon']+=1
            continue
        max_residual=list(map(max,max_residual,residuals))
        shared_count+=1
        weight_sum+=event['weight']
        for stream,collection in zip(bodies,[
            {'particles':[dict(p,status=4) for p in before]},
            {'hadronized_particles':p6},
            {'hadronized_particles':p8},
        ]):
            row=dict(type='event',attempt=shared_count,source_attempt=event['attempt'],
                     weight=event['weight'],**collection)
            stream.write(json.dumps(row,allow_nan=False)+'\n')
    assert not dump.read().strip()
assert shared_count
for name,path in zip(stages,body_paths):
    with (args.directory/(name+'.jsonl')).open('w') as f,path.open() as body:
        def emit(row):
            f.write(json.dumps(row,allow_nan=False)+'\n')
        schema='jewel-parton-development-v1' if name=='before' else 'jewel-hadron-development-v1'
        emit(dict(type='run',schema=schema,mode=source_header['mode'],
                  attempts_requested=shared_count,fragmentation_stage=name,
                  input_run=str(args.input.resolve())))
        shutil.copyfileobj(body,f)
        emit(dict(type='summary',attempts=shared_count,accepted=shared_count,
                  generator_weight_sum=weight_sum))
    path.unlink()
def digest(path):
    with path.open('rb') as f:
        return hashlib.file_digest(f,'sha256').hexdigest()

summary=dict(native_event_sha256=digest(args.input),pyexec_sha256=digest(args.pyexec),
             packed_input_sha256=digest(args.directory/'pyexec-input.dat'),source_events=source_count,shared_events=shared_count,excluded=dict(excluded),
             max_component_residual_gev=dict(pythia6=max_residual[0],pythia8=max_residual[1]))
(args.directory/'audit.json').write_text(json.dumps(summary,indent=2)+'\n')
print(summary)
