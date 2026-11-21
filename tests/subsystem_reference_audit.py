"""Compare full initialized native records with frozen JEWEL FINDVIRTUALITIES."""
import hashlib,json,math,subprocess,sys
from pathlib import Path
reference,sample,fixtures,output=sys.argv[1:]
rows=[json.loads(line) for line in Path(sample).open()]
events=[r for r in rows if r['type']=='event' and r.get('initialization',{}).get('error','missing') is None]
payload=Path(fixtures).read_text()
inputs=iter(payload.splitlines());assert next(inputs)=='CONFIG';next(inputs)
assert next(inputs)=='BCONFIG';next(inputs);prepared=[]
for event in events:
    assert next(inputs)==('SEQFLAV' if rows[0].get('mpi_channel_policy')=='outgoing-flavour' else 'SEQINIT');config=list(map(float,next(inputs).split()))
    n,npairs,fsr=map(int,next(inputs).split());assert npairs==event['initialization']['pairs']
    pairs=[list(map(int,next(inputs).split())) for _ in range(npairs)]
    records=[list(map(float,next(inputs).split())) for _ in range(n)]
    assert all(len(record)==26 for record in records)
    assert next(inputs)=='-1';assert int(next(inputs))==event['initialization']['seed']
    prepared.append((pairs,records));assert n==len(event['initialization']['records'])
assert next(inputs,None) is None
run=subprocess.run([reference],input=payload,text=True,capture_output=True,timeout=120)
if run.returncode:raise RuntimeError((run.returncode,run.stderr[-2000:]))
lines=iter(run.stdout.splitlines());failures=[];values=0;maximum=0;draw_errors=0
native_residual=0.;reference_residual=0.
for event,(pairs,before) in zip(events,prepared):
    reference_records=[]
    init=event['initialization'];draws=int(next(lines));draw_errors+=draws!=init['draws']
    if draws!=init['draws']:failures.append(dict(attempt=event['attempt'],field='draws',native=init['draws'],reference=draws))
    for record,actual in enumerate(init['records'],1):
        expected=list(map(float,next(lines).split()));assert len(actual)==len(expected)==26
        reference_records.append(expected)
        for column,(x,y) in enumerate(zip(actual,expected)):
            values+=1;maximum=max(maximum,abs(x-y))
            discrete=column<5 or 21<=column<=24
            okay=x==y if discrete else math.isfinite(x) and math.isfinite(y) and math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13)
            if not okay:failures.append(dict(attempt=event['attempt'],record=record,column=column,native=x,reference=y))
    for first,second in pairs:
        for column in range(5,9):
            initial=before[first-1][column]+before[second-1][column]
            native_residual=max(native_residual,abs(initial-init['records'][first-1][column]-init['records'][second-1][column]))
            reference_residual=max(reference_residual,abs(initial-reference_records[first-1][column]-reference_records[second-1][column]))
assert next(lines,None) is None
report=dict(mpi_channel_policy=rows[0].get('mpi_channel_policy','source-primary'),scope='Full-record explicit primary/MPI sequence replay; not Pythia6 generation or cascade equivalence',native_maximum_pair_p4_residual_gev=native_residual,reference_maximum_pair_p4_residual_gev=reference_residual,events=len(events),pairs=sum(e['initialization']['pairs'] for e in events),values=values,max_absolute_error=maximum,draw_count_mismatches=draw_errors,failures=failures,relative_tolerance=2e-12,absolute_tolerance=2e-13,hashes={p:hashlib.sha256(Path(p).read_bytes()).hexdigest() for p in [reference,sample,fixtures,__file__]})
Path(output).write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items() if k not in ['hashes','failures']},indent=2))
if failures:print(failures[:10]);raise SystemExit(f'{len(failures)} mismatches')
