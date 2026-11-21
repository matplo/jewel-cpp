"""Import JEWEL HepMC2 events, optionally restoring explicitly logged discards.

Uses hepyy's native HepMC3 reader. This is an analysis interchange format, not
native-generator output or evidence of matched MC settings.
"""
import argparse
import hashlib
import json
import math
import re
from pathlib import Path
import tempfile


def identity(path):
    digest=hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda:stream.read(1024*1024),b''):digest.update(chunk)
    return dict(path=str(path.resolve()),sha256=digest.hexdigest())


def discard_ledger(settings,log):
    """Require a complete, unit-weight PP ledger; never infer unknown failures."""
    if settings.get('WEIGHTED')!='F' or settings.get('ISOCHANNEL')!='PP':
        raise ValueError('attempt ledger requires unweighted PP generation')
    text=log.read_text()
    def single(pattern):
        values=re.findall(pattern,text,re.M)
        if len(values)!=1:raise ValueError('missing or repeated run-log ledger field')
        return int(values[0])
    for key in ['NEVENT','NJOB']:
        if single(r'^\s*'+key+r'\s*=\s*(\d+)\s*$')!=int(settings[key]):
            raise ValueError('run log does not match '+key)
    discarded=single(r'^\s*total number of discarded events:\s*(\d+)\s*$')
    if single(r'^\s*number of events for which conversion to hepmc failed:\s*(\d+)\s*$'):
        raise ValueError('HepMC conversion losses prevent complete attempt ledger')
    indices=[int(x) for x in re.findall(r'^\s*(?:discard event|PYTHIA discards event)\s+(\d+)(?:\s+\(error number[^\n]*)?\s*$',text,re.M)]
    if len(indices)!=discarded or len(set(indices))!=discarded or any(i<1 or i>int(settings['NEVENT']) for i in indices):
        raise ValueError('discarded attempt indices are incomplete or inconsistent')
    return set(indices)


def import_events(source,cards,medium_cards,mode,output,run_log=None):
    if output.exists():raise ValueError('output already exists')
    settings={}
    for line in cards.read_text().splitlines():
        line=line.split('#',1)[0].split('!',1)[0].strip()
        if not line:continue
        key,value=line.split(None,1)
        if key in settings:raise ValueError('duplicate card: '+key)
        settings[key]=value.strip()
    count=int(settings['NEVENT'])
    if count<=0:raise ValueError('positive NEVENT required')
    discarded=discard_ledger(settings,run_log) if run_log is not None else set()
    scope='attempted-events' if run_log is not None else 'written-events'
    if settings.get('KEEPRECOILS') not in ('T','F'):raise ValueError('explicit KEEPRECOILS T/F required')
    if settings.get('WRITEDUMMIES')!='F':raise ValueError('WRITEDUMMIES F required; dummy removal is not implemented')
    with source.open('rb') as stream:
        stream.seek(max(0,source.stat().st_size-4096))
        if not stream.read().rstrip().endswith(b'HepMC::IO_GenEvent-END_EVENT_LISTING'):
            raise ValueError('missing complete HepMC2 end marker')
    import hepyy
    hepyy.load('hepmc3')
    import cppyy
    cppyy.include('HepMC3/ReaderAsciiHepMC2.h')
    cppyy.include('HepMC3/GenCrossSection.h')
    H=cppyy.gbl.HepMC3
    header=dict(type='run',schema='jewel-fortran-hepmc-analysis-v1',generator_family='JEWEL 2.6.0 Fortran',
        mode=mode,attempts_requested=count,denominator_scope=scope,
        failed_attempts_known=run_log is not None,keep_recoils=settings['KEEPRECOILS']=='T',
        importer=identity(Path(__file__)),reference_input=identity(source),cards={**identity(cards),'text':cards.read_text()},
        medium_cards={**identity(medium_cards),'text':medium_cards.read_text()},
        output_policy={key:settings.get(key) for key in ['DOSUBTRACTION','PUTONSHELL','RSUB','WRITESCATCEN','WRITEDUMMIES']},
        selection_input='HepMC status 1 only; neutrinos retained here and excluded by common analysis',
        normalization='per logged GENEVENT attempt with explicit discarded indices' if run_log is not None else 'per written event; internal retries/failures unavailable; not matched to native per-attempt normalization')
    if run_log is not None:header['reference_ledger']={**identity(run_log),'discarded_attempts':sorted(discarded),'policy':'unweighted PP; conversion losses forbidden; each logged discard has unit weight'}
    reader=H.ReaderAsciiHepMC2(str(source));seen=set();weights=[];last_xsec=None
    temp=None
    try:
        with tempfile.NamedTemporaryFile(mode='w',dir=output.parent,prefix=output.name+'.',delete=False) as out:
            temp=Path(out.name)
            def emit(record):out.write(json.dumps(record,allow_nan=False)+'\n')
            emit(header)
            next_attempt=1
            def emit_discard(attempt):
                emit(dict(type='reference_failure',attempt=attempt,weight=1.,stage='reference',reason='JEWEL logged discarded event'))
            while True:
                event=H.GenEvent();reader.read_event(event)
                if reader.failed():break
                if event.momentum_unit()!=H.Units.GEV:raise ValueError('expected GEV input')
                number=event.event_number()
                if number in seen:raise ValueError('duplicate HepMC event number')
                seen.add(number)
                weight=list(event.weights())
                if len(weight)!=1 or not math.isfinite(weight[0]):raise ValueError('expected one finite event weight')
                if run_log is not None and weight[0]!=1.:raise ValueError('attempt ledger requires unit event weights')
                while next_attempt in discarded:
                    emit_discard(next_attempt);next_attempt+=1
                weights.append(float(weight[0]));particles=[]
                for p in event.particles():
                    if p.status()!=1:continue
                    v=p.momentum();momentum=[v.px(),v.py(),v.pz(),v.e()]
                    if not all(math.isfinite(x) for x in momentum):raise ValueError('nonfinite particle momentum')
                    particles.append(dict(id=p.id(),pdg=p.pid(),final=True,momentum=momentum))
                xsec=event.cross_section();event_xsec=None
                if xsec:
                    last_xsec=event_xsec=dict(value_pb=xsec.xsec(),error_pb=xsec.xsec_err())
                    if not all(math.isfinite(x) for x in last_xsec.values()):raise ValueError('nonfinite cross section')
                emit(dict(type='event',attempt=next_attempt,reference_event_number=number,weight=weight[0],
                          final_particles=particles,cross_section=event_xsec))
                next_attempt+=1
            while next_attempt in discarded:
                emit_discard(next_attempt);next_attempt+=1
            if next_attempt!=count+1 or len(weights)!=count-len(discarded):raise ValueError(f'expected {count} events, read {len(weights)}')
            emit(dict(type='summary',attempts=count,accepted=len(weights),evolution_failures=None,generation_failures=None,discarded_events=len(discarded) if run_log is not None else None,
                      generator_weight_sum=math.fsum(weights)+len(discarded),generator_weight_sum2=math.fsum(w*w for w in weights)+len(discarded),
                      last_cross_section=last_xsec,denominator_scope=scope,failed_attempts_known=run_log is not None))
        temp.replace(output)
    finally:
        reader.close()
        if temp is not None and temp.exists():temp.unlink()


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('input',type=Path);p.add_argument('--cards',type=Path,required=True)
    p.add_argument('--medium-cards',type=Path,required=True)
    p.add_argument('--mode',choices=['vacuum','medium','zero-interaction'],required=True)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--run-log',type=Path,help='Complete JEWEL log: restore discarded unweighted PP attempts and normalize per attempt')
    a=p.parse_args();import_events(a.input,a.cards,a.medium_cards,a.mode,a.output,a.run_log)
