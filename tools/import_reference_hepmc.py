"""Import written JEWEL HepMC2 events, without inventing failed-attempt counts.

Uses hepyy's native HepMC3 reader. This is an analysis interchange format, not
native-generator output or evidence of matched MC settings.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import tempfile


def identity(path):
    digest=hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda:stream.read(1024*1024),b''):digest.update(chunk)
    return dict(path=str(path.resolve()),sha256=digest.hexdigest())


def import_events(source,cards,medium_cards,mode,output):
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
        mode=mode,attempts_requested=count,denominator_scope='written-events',
        failed_attempts_known=False,keep_recoils=settings['KEEPRECOILS']=='T',
        importer=identity(Path(__file__)),reference_input=identity(source),cards={**identity(cards),'text':cards.read_text()},
        medium_cards={**identity(medium_cards),'text':medium_cards.read_text()},
        output_policy={key:settings.get(key) for key in ['DOSUBTRACTION','PUTONSHELL','RSUB','WRITESCATCEN','WRITEDUMMIES']},
        selection_input='HepMC status 1 only; neutrinos retained here and excluded by common analysis',
        normalization='per written event; internal retries/failures unavailable; not matched to native per-attempt normalization')
    reader=H.ReaderAsciiHepMC2(str(source));seen=set();weights=[];last_xsec=None
    temp=None
    try:
        with tempfile.NamedTemporaryFile(mode='w',dir=output.parent,prefix=output.name+'.',delete=False) as out:
            temp=Path(out.name)
            def emit(record):out.write(json.dumps(record,allow_nan=False)+'\n')
            emit(header)
            while True:
                event=H.GenEvent();reader.read_event(event)
                if reader.failed():break
                if event.momentum_unit()!=H.Units.GEV:raise ValueError('expected GEV input')
                number=event.event_number()
                if number in seen:raise ValueError('duplicate HepMC event number')
                seen.add(number)
                weight=list(event.weights())
                if len(weight)!=1 or not math.isfinite(weight[0]):raise ValueError('expected one finite event weight')
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
                emit(dict(type='event',attempt=len(weights),reference_event_number=number,weight=weight[0],
                          final_particles=particles,cross_section=event_xsec))
            if len(weights)!=count:raise ValueError(f'expected {count} events, read {len(weights)}')
            emit(dict(type='summary',attempts=count,accepted=count,evolution_failures=None,generation_failures=None,
                      generator_weight_sum=math.fsum(weights),generator_weight_sum2=math.fsum(w*w for w in weights),
                      last_cross_section=last_xsec,denominator_scope='written-events',failed_attempts_known=False))
        temp.replace(output)
    finally:
        reader.close()
        if temp is not None and temp.exists():temp.unlink()


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('input',type=Path);p.add_argument('--cards',type=Path,required=True)
    p.add_argument('--medium-cards',type=Path,required=True)
    p.add_argument('--mode',choices=['vacuum','medium'],required=True)
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();import_events(a.input,a.cards,a.medium_cards,a.mode,a.output)
