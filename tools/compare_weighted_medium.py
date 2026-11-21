"""Compare compensated jet yields per unit accepted hard-event weight."""
import argparse
from bisect import bisect_right
import hashlib
from html import escape
import json
import math
from pathlib import Path

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('analysis',type=Path)
p.add_argument('output',type=Path)
p.add_argument('--html',type=Path)
p.add_argument('--previous-report',type=Path,help='Guarded reference comparison made with the same baselines')
p.add_argument('--pilot-report',type=Path,help='Independent corrected pilot to overlay on the new ratios')
p.add_argument('--omit-fortran-medium',action='store_true',help='Inputs are C++ medium, C++ vacuum and Fortran vacuum')
a=p.parse_args()
data=json.loads(a.analysis.read_text())
samples=data['samples']
labels=['cpp_medium','cpp_vacuum','fortran_medium','fortran_vacuum']
expected=['medium','vacuum']*2
matched_cpp=data.get('matched_cpp_attempts_by_seed')
if a.omit_fortran_medium:
    assert len(samples)==3
    samples.insert(2,None)
assert [s['run']['mode'] if s else 'medium' for s in samples]==expected
assert [s['run']['schema'] if s else 'jewel-fortran-hepmc-analysis-v1' for s in samples]==['jewel-hadron-development-v1']*2+['jewel-fortran-hepmc-analysis-v1']*2
def cards(text):
    return dict(line.split(None,1) for raw in text.splitlines()
                if (line:=raw.partition('#')[0].partition('!')[0].strip()))

assert samples[0]['run']['centrality_percent']==[0,10]
for sample in samples[:2]:
    settings=sample['run']['pythia_settings']
    assert 'PhaseSpace:bias2Selection = on' in settings
    assert 'PhaseSpace:bias2SelectionPow = 5' in settings
for sample,scale in zip(samples[2:],[1,0]):
    if sample is None:
        continue
    run=sample['run']
    run_cards=cards(run['cards']['text'])
    medium_cards=cards(run['medium_cards']['text'])
    assert run_cards['WEIGHTED']=='T' and float(run_cards['WEXPO'])==5
    assert float(run_cards['XSECSCALEFAC'])==scale, 'Fortran scattering scale does not match comparison mode'
    assert float(medium_cards['CENTRMIN'])==0 and float(medium_cards['CENTRMAX'])==10

pt_edges=[10,20,30,40,60,80,100,150,float('inf')]

def measure(sample,key,edges,pt_range=None,influences=None):
    events=[e for e in sample['events'] if e['accepted']]
    wsum=math.fsum(e['weight'] for e in events)
    assert wsum>0 and all(e['weight']>0 for e in events)
    values=[[] for _ in edges[1:]]
    for e in events:
        n=[0]*len(values)
        for jet in e['jets']:
            if pt_range and not pt_range[0]<=jet['pt']<pt_range[1]:
                continue
            v=jet[key]
            if v is None:
                continue
            index=bisect_right(edges,v)-1
            if 0<=index<len(values):
                n[index]+=1
        for entries,count in zip(values,n):
            entries.append((e['attempt']-1,e['weight'],count))
    out=[]
    for entries in values:
        mean=math.fsum(w*n for _,w,n in entries)/wsum
        variance=math.fsum((w*(n-mean))**2 for _,w,n in entries)/wsum**2
        out.append(dict(yield_per_weight=mean,standard_error=math.sqrt(variance*len(events)/(len(events)-1)),
                        raw_jets=sum(n for _,_,n in entries)))
        if influences is not None:
            per_attempt=[0.]*sample['summary']['attempts']
            for attempt,w,n in entries:
                per_attempt[attempt]=w*(n-mean)/wsum
            influences.append(per_attempt)
    return out

def ratio(a,b):
    if a is None or b is None:
        return None
    x,y=a['yield_per_weight'],b['yield_per_weight']
    if y==0:
        return None
    return dict(value=x/y,standard_error=math.hypot(a['standard_error']/y,x*b['standard_error']/y**2))

def rows(key,edges,pt_range=None):
    cpp_influences=[[],[]]
    groups=[measure(s,key,edges,pt_range,cpp_influences[i] if matched_cpp and i<2 else None)
            if s else [None]*(len(edges)-1) for i,s in enumerate(samples)]
    result=[]
    for i,(lo,hi) in enumerate(zip(edges,edges[1:])):
        cm,cv,fm,fv=[g[i] for g in groups]
        cpp=ratio(cm,cv)
        if matched_cpp and cpp:
            a,b=(group[i] for group in cpp_influences)
            assert len(a)==len(b)>1
            variance=math.fsum((m-cpp['value']*v)**2 for m,v in zip(a,b))
            cpp['standard_error']=math.sqrt(variance*len(a)/(len(a)-1))/cv['yield_per_weight']
        fortran=ratio(fm,fv)
        gap=dict(value=fortran['value']-cpp['value'],standard_error=math.hypot(fortran['standard_error'],cpp['standard_error'])) if fortran and cpp else None
        result.append(dict(low=lo,high=hi if math.isfinite(hi) else None,yields=dict(zip(labels,[cm,cv,fm,fv])),
            cpp_medium_over_vacuum=cpp,fortran_medium_over_vacuum=fortran,medium_vacuum_gap=gap,
            cpp_over_fortran_medium=ratio(cm,fm),cpp_over_fortran_vacuum=ratio(cv,fv)))
    return result

def leave_one_seed_out(key,lo,hi,jet_pt=None):
    seeds=list(matched_cpp)
    shard_totals=[]
    for sample in samples:
        shards=sample['run']['independent_shards']
        assert [str(s['seed']) for s in shards]==seeds
        totals=[]
        first=0
        for shard in shards:
            last=first+shard['summary']['attempts']
            events=[e for e in sample['events'][first:last] if e['accepted']]
            weight=math.fsum(e['weight'] for e in events)
            weighted_jets=math.fsum(e['weight'] for e in events for jet in e['jets']
                                    if jet[key] is not None and lo<=jet[key]<hi
                                    and (jet_pt is None or jet_pt[0]<=jet['pt']<jet_pt[1]))
            totals.append((weighted_jets,weight))
            first=last
        assert first==len(sample['events'])
        shard_totals.append(totals)
    gaps=[]
    for i,seed in enumerate(seeds):
        cm,cv,fm,fv=(math.fsum(n for j,(n,_) in enumerate(totals) if j!=i)/
                     math.fsum(w for j,(_,w) in enumerate(totals) if j!=i)
                     for totals in shard_totals)
        gaps.append(dict(omitted_seed=int(seed),fortran_minus_cpp=fm/fv-cm/cv))
    return gaps

def bins(key):
    items=samples[0]['distributions'][key]
    return [items[0]['low'],*[b['high'] for b in items]]

spectra={'Jet pT':rows('pt',pt_edges)}
for key in ('mass','girth','ptd','zg','groomed_mass'):
    spectra[key]=rows(key,bins(key))
    for lo,hi in [(20,40),(40,60),(60,100)]:
        spectra[f'{key} | jet pT {lo}–{hi} GeV']=rows(key,bins(key),(lo,hi))

reports={}
for label,s in zip(labels,samples):
    if s is None:
        reports[label]=None
        continue
    weights=[e['weight'] for e in s['events'] if e['accepted']]
    reports[label]={k:s[k] for k in ('run','summary','selected_jets','input_sha256','failure_reasons')}
    reports[label]['effective_accepted_events']=math.fsum(weights)**2/math.fsum(w*w for w in weights)
report=dict(scope='0–10% Pb–Pb geometry with pp hard channel, and matched vacuum baselines',
    normalization='sum of inverse-bias jet weights divided by sum of accepted event weights in each sample',
    uncertainty='event-level delta method, independent-sample approximation for ratios',
    fortran_medium_available=not a.omit_fortran_medium,selection=data['selection'],analysis_versions=data['analysis_versions'],
    analysis_sha256=hashlib.sha256(a.analysis.read_bytes()).hexdigest(),samples=reports,spectra=spectra)
if a.previous_report:
    previous=json.loads(a.previous_report.read_text())
    assert previous['selection']==report['selection']
    for label in ('cpp_medium','cpp_vacuum','fortran_vacuum'):
        assert previous['samples'][label]['input_sha256']==reports[label]['input_sha256']
    report['previous_fortran_medium']=previous['samples']['fortran_medium']
    for key, current_rows in spectra.items():
        previous_rows=previous['spectra'][key]
        assert len(current_rows)==len(previous_rows)
        for row, old in zip(current_rows,previous_rows):
            assert (row['low'],row['high'])==(old['low'],old['high'])
            row['previous_fortran_medium_over_vacuum']=old['fortran_medium_over_vacuum']
if matched_cpp:
    report['matched_cpp_attempts_by_seed']=matched_cpp
    report['cpp_mpi_pt0_gev_by_seed']=data.get('cpp_mpi_pt0_gev_by_seed')
    report['uncertainty']=('Event-level delta method. C++ medium/vacuum uses paired hard draws; '
                           'Fortran medium/vacuum and cross-implementation comparisons use independent-sample errors.')
if a.pilot_report:
    pilot=json.loads(a.pilot_report.read_text())
    assert pilot['selection']==report['selection']
    assert pilot['samples']['fortran_medium']['run']['cascade_nan_fix']
    report['pilot_metadata']=dict(seed_note=data.get('pilot_seed_note'),analysis_sha256=pilot['analysis_sha256'],
                                  selected_jets={k:v['selected_jets'] for k,v in pilot['samples'].items()})
    for key,current_rows in spectra.items():
        pilot_rows=pilot['spectra'][key]
        assert len(current_rows)==len(pilot_rows)
        for row,old in zip(current_rows,pilot_rows):
            assert (row['low'],row['high'])==(old['low'],old['high'])
            for label in ('cpp_medium_over_vacuum','fortran_medium_over_vacuum'):
                row['pilot_'+label]=old[label]
    report['focus']={}
    for name,key,low,jet_pt in [('Jet pT','pt',60,None),('ptd | jet pT 40–60 GeV','ptd',.6,(40,60))]:
        row=next(row for row in spectra[name] if row['low']==low)
        report['focus'][name]=dict(row)
        if matched_cpp:
            report['focus'][name]['leave_one_seed_out']=leave_one_seed_out(key,low,row['high'],jet_pt)
a.output.write_text(json.dumps(report,indent=2,allow_nan=False)+'\n')
if a.html:
    guarded=reports['fortran_medium'] and reports['fortran_medium']['run'].get('cascade_nonfinite_guard')
    fixed=reports['fortran_medium'] and reports['fortran_medium']['run'].get('cascade_nan_fix')
    context=''
    if fixed:
        context='<h2>Where the cascade first became nonfinite</h2><p>'+escape(fixed['finding'])+'</p><p>'+escape(fixed['validation'])+'</p>'
        if fixed.get('trace'):
            context+=('<details><summary>Original Fortran trace</summary>'
                      '<p>The BEFORE SQRT fields are transfer index, outgoing line, incoming line, p32, p42, '
                      'outgoing px before rescaling, original centre-of-mass pz, and whether the incoming momentum is finite.</p>'
                      '<pre>'+escape('\n'.join(fixed['trace']))+'</pre></details>')
    if fixed and a.previous_report:
        pt=spectra['Jet pT']
        first=pt[0]
        mid=next(row for row in pt if row['low']==60)
        ptd=next(row for row in spectra['ptd | jet pT 40–60 GeV'] if row['low']==.6)
        def value(row,key):
            return f"{row[key]['value']:.3f}"
        stable=all(row['fortran_medium_over_vacuum']['value']<1
                   for row in pt if 20<=row['low']<100)
        suppression='The corrected Fortran ratio is below unity in every 20–100 GeV bin.' if stable else ''
        context+=('<h2>What changed</h2><p>For jet pT of 10–20 GeV, the Fortran ratio moved from '
                  + value(first,'previous_fortran_medium_over_vacuum')+' to '+value(first,'fortran_medium_over_vacuum')
                  + '. At 60–80 GeV it moved from '+value(mid,'previous_fortran_medium_over_vacuum')
                  + ' to '+value(mid,'fortran_medium_over_vacuum')+', compared with '
                  + value(mid,'cpp_medium_over_vacuum')+' in C++. '+suppression+'</p>'
                  + '<p>In the 40–60 GeV jet slice, the medium/vacuum ratio in the pTD bin 0.6–0.7 is '
                  + value(ptd,'fortran_medium_over_vacuum')+' after the fix and '
                  + value(ptd,'cpp_medium_over_vacuum')+' in C++. The before/after view shows the errors and other bins.</p>')
    if a.pilot_report:
        focused=[]
        for key,title in [('Jet pT','Jet pT 60–80 GeV'),('ptd | jet pT 40–60 GeV','pTD 0.6–0.7 at jet pT 40–60 GeV')]:
            row=report['focus'][key]
            cpp=row['cpp_medium_over_vacuum']
            f=row['fortran_medium_over_vacuum']
            gap=row['medium_vacuum_gap']
            fixed_gap=row['pilot_fortran_medium_over_vacuum']['value']-row['pilot_cpp_medium_over_vacuum']['value']
            sensitivity=''
            if row.get('leave_one_seed_out'):
                points=[g['fortran_minus_cpp'] for g in row['leave_one_seed_out']]
                sensitivity=f" Omitting one seed at a time gives gaps from {min(points):+.3f} to {max(points):+.3f}."
            focused.append(f"<li><strong>{title}</strong> &nbsp; C++ {cpp['value']:.3f} ± {cpp['standard_error']:.3f}, "
                           f"Fortran {f['value']:.3f} ± {f['standard_error']:.3f}. "
                           f"Fortran minus C++ {gap['value']:+.3f} ± {gap['standard_error']:.3f} "
                           f"(original pilot {fixed_gap:+.3f}).{sensitivity}</li>")
        trace=context
        context='<h2>Comparison in the two pilot bins</h2>'
        if matched_cpp:
            attempts=[v['summary']['attempts'] for v in reports.values()]
            if len(set(attempts))==1:
                context+=f'<p>{attempts[0]:,} attempts per sample across {len(matched_cpp)} fresh seed shards.</p>'
        context+='<ul>'+''.join(focused)+'</ul>'
        gaps=[row['medium_vacuum_gap'] for row in report['focus'].values()]
        if all(abs(gap['value'])<gap['standard_error'] for gap in gaps):
            context+='<p>Both differences are within one standard error of zero in the larger comparison.</p>'
        ptd_counts=[item['raw_jets'] for item in report['focus']['ptd | jet pT 40–60 GeV']['yields'].values()]
        context+=f'<p>The highlighted pTD bin contains {min(ptd_counts):,}–{max(ptd_counts):,} raw jets per sample.</p>'
        if data.get('pilot_seed_note'):
            context+='<p class="seed-note">'+escape(data['pilot_seed_note'])+'</p>'
        pt0=data.get('cpp_mpi_pt0_gev_by_seed',{})
        if len(set(pt0.values()))>1:
            values=', '.join(f'{seed} ({value:.3f} GeV)' for seed,value in pt0.items())
            context+=('<details><summary>C++ MPI calibration by seed</summary>'
                      '<p>Native MPI calibrates pT0 stochastically. Medium and vacuum share the same value within each seed. '
                      'The seed-omission ranges above include dropping the shard with a different calibration.</p>'
                      '<p>'+values+'</p></details>')
        if trace:
            context+='<details><summary>Fortran fix and validation</summary>'+trace+'</details>'
    counts='<details class="samples"><summary>Sample counts</summary><ul>'
    for label,sample in [*reports.items(),('fortran_medium_guarded',report.get('previous_fortran_medium'))]:
        if sample:
            names={'cpp_medium':'C++ medium','cpp_vacuum':'C++ vacuum','fortran_medium':'Fortran medium',
                   'fortran_vacuum':'Fortran vacuum','fortran_medium_guarded':'Fortran medium (previous guard)'}
            summary=sample['summary']
            counts+=(f"<li>{names[label]}: {summary['accepted']:,} / {summary['attempts']:,} accepted, "
                     f"{sample['selected_jets']:,} selected jets, {sample['effective_accepted_events']:,.0f} effective weighted events</li>")
    counts+='</ul></details>'
    payload=json.dumps(spectra,allow_nan=False).replace('<','\\u003c')
    a.html.write_text('''<!doctype html>
<html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>Central Pb–Pb geometry compared with vacuum</title>
<style>
body{margin:2em auto;max-width:74em;padding:0 1em;font:16px/1.5 system-ui;color:#16304a}
h1{font-size:clamp(1.9em,4vw,3em);line-height:1.08;max-width:23em}h2,h3{line-height:1.25}
.controls{display:flex;gap:.8em 1.1em;flex-wrap:wrap;align-items:center;margin-bottom:1em}
.controls label{white-space:nowrap}button,select{border:1px solid #b4c8d9;border-radius:6px;background:white;color:inherit;padding:.33em .5em}
button{cursor:pointer}.chart{border:1px solid #d6e2ee;border-radius:10px;padding:.4em 0 .6em;background:#fff;overflow-x:auto}
svg{display:block;width:100%;min-width:540px;max-height:430px}.ticks{font:12px system-ui;fill:#40566d}.axis{font:14px system-ui;fill:#16304a}
svg g:focus circle,svg g:hover circle{stroke:#111;stroke-width:2.7}
.legend{display:flex;flex-wrap:wrap;justify-content:center;gap:.25em 1em;list-style:none;padding:0;margin:0;font-size:.9rem}
.legend span{display:inline-block;width:.58em;height:.58em;border:2px solid;border-radius:50%;margin-right:.4em}
small{display:block;margin-top:.9em}.samples,details{margin:.75em 0}
[hidden]{display:none!important}
table{width:100%;border-collapse:collapse}td,th{text-align:left;padding:.65em;border-bottom:1px solid #ccd6e0}
select,button{font:inherit} small{color:#42566b}
pre,.table-scroll{overflow:auto}summary{cursor:pointer}
</style>
<h1>Central Pb–Pb geometry compared with vacuum</h1>
<p>0–10% centrality, pp hard channel and pThat bias power 5. 5.02 TeV, anti-kT R = 0.4, jet pT ≥ 10 GeV and |η| &lt; 2. Medium recoils are retained without thermal subtraction. Yields use inverse-bias weights per unit accepted event weight, with no nuclear overlap rescaling.</p>
''' + ('<p>Fortran medium is unavailable. The medium/vacuum result shown here uses the C++ pair.</p>' if a.omit_fortran_medium else '<p>The Fortran medium run rejects nonfinite cascade states. Ratios describe the retained events and are diagnostic.</p>' if guarded else '') + context + counts + '''
<h2>Jet observables</h2>
<div class="controls">
<label>Observable <select id="observable"></select></label>
<label>Display <select id="display"><option value="ratios">Medium / vacuum</option><option value="gap">Fortran minus C++ ratios</option><option value="generators">C++ / Fortran yields</option><option value="history">Fortran before / after fix</option><option value="yields">Weighted bin yields</option></select></label>
<label><input id="pilot" type="checkbox"> Overlay original pilot</label>
<label><input id="errors" type="checkbox" checked> Error bars</label>
<label>Y scale <select id="scale"><option value="auto">Auto</option><option value="linear">Linear</option><option value="log">Log</option></select></label>
<button type="button" data-observable="Jet pT">Jet pT</button>
<button type="button" data-observable="ptd | jet pT 40–60 GeV">pTD at 40–60 GeV</button>
</div>
<div id="plot" class="chart"></div>
<section id="secondary" hidden><h3>pTD at jet pT 40–60 GeV</h3><div id="plot-ptd" class="chart"></div></section>
<p><small>''' + escape(report['uncertainty']) + ''' Error bars show one standard error. Log plots place zero values and error bars crossing zero at the lower edge. Points represent whole bins. Hover over a point or use the data table for values.</small></p>
<details><summary>Data table for current observable</summary>
<div class="table-scroll"><table><thead><tr></tr></thead><tbody></tbody></table></div>
<p><small>An em dash indicates an undefined ratio.</small></p></details>
<script>
const spectra = '''+payload+''';
const menu = document.querySelector('#observable');
const display = document.querySelector('#display');
const overlay = document.querySelector('#pilot');
const showErrors = document.querySelector('#errors');
const scale = document.querySelector('#scale');
const comparePilot = Object.hasOwn(spectra['Jet pT'][0], 'pilot_cpp_medium_over_vacuum');
const hasGuarded = Object.hasOwn(spectra['Jet pT'][0], 'previous_fortran_medium_over_vacuum');
if(!comparePilot) overlay.closest('label').hidden=true;
if(!hasGuarded) display.querySelector('[value=history]').remove();
if(hasGuarded) display.value='history';
for (const key of Object.keys(spectra)) menu.add(new Option(key,key));
document.querySelectorAll('[data-observable]').forEach(button => button.addEventListener('click', () => {
  menu.value=button.dataset.observable;
  render();
}));

const cpp = ['C++', '#087c83'];
const fortran = ['Fortran', '#a63455'];
const ratios = [
  ['cpp_medium_over_vacuum', ...cpp],
  ['fortran_medium_over_vacuum', ...fortran],
];
const modes = {
  ratios,
  gap: [['medium_vacuum_gap','Fortran − C++','#6341a0']],
  generators: [
    ['cpp_over_fortran_medium','C++ / Fortran medium','#087c83'],
    ['cpp_over_fortran_vacuum','C++ / Fortran vacuum','#6341a0'],
  ],
  yields: [
    ['cpp_medium','C++ medium','#087c83'],
    ['cpp_vacuum','C++ vacuum','#4378b4'],
    ['fortran_medium','Fortran medium','#a63455'],
    ['fortran_vacuum','Fortran vacuum','#a75c24'],
  ],
  history: [['previous_fortran_medium_over_vacuum','Guarded Fortran','#5f686e'],...ratios],
};
function seriesFor(mode, pilot){
  const series=modes[mode];
  if(mode==='ratios' && pilot){
    return [...series,...ratios.map(([key,label,color])=>['pilot_'+key,label+' pilot',color,true])];
  }
  return series;
}
function point(row,key,mode){
  if(mode==='yields'){
    const y=row.yields[key];
    return y && {value:y.yield_per_weight,standard_error:y.standard_error};
  }
  return row[key];
}
const format = v => v ? v.value.toPrecision(3)+' ± '+v.standard_error.toPrecision(2) : '—';
const binLabel = r => r.low+'–'+(r.high==null?'∞':Number(r.high.toFixed(5)));

function svgNode(tag, attrs={}, label){
  const element=document.createElementNS('http://www.w3.org/2000/svg',tag);
  for(const [k,v] of Object.entries(attrs)) element.setAttribute(k,v);
  if(label!==undefined) element.textContent=label;
  return element;
}
function drawPlot(target, rows, series, mode, errors, axis, highlight){
  const baseline=mode==='yields'?null:mode==='gap'?0:1;
  const points=series.map(([key])=>rows.map(r=>point(r,key,mode)));
  const limits=points.flat().filter(Boolean).flatMap(p=>[
    p.value-(errors?p.standard_error:0),p.value+(errors?p.standard_error:0),
  ]);
  if(baseline!==null) limits.push(baseline);
  if(!limits.length){
    target.textContent='No defined values for this view.';
    return;
  }
  const min=Math.min(...limits),max=Math.max(...limits);
  const positivePoints=points.flat().filter(p=>p && p.value>0).map(p=>p.value);
  if(baseline===1) positivePoints.push(1);
  const lowestPositive=Math.min(...positivePoints);
  const log=mode!=='gap' && positivePoints.length>0 &&
    (scale.value==='log' || (scale.value==='auto' && max/lowestPositive>25));
  const pad=(max-min)*.08||(Math.abs(max)||1)*.08;
  const low=log?10**(Math.floor(Math.log10(lowestPositive))-.15):mode==='gap'?min-pad:Math.max(0,min-pad);
  const high=log?10**(Math.ceil(Math.log10(max)*2+.12)/2):max+pad;
  const width=760,height=335,left=74,right=18,top=14,bottom=72;
  const end=width-right,base=height-bottom;
  const step=(end-left)/rows.length;
  const scaled=v=>log?Math.log10(Math.max(low,v)):v;
  const startValue=scaled(low),span=scaled(high)-startValue;
  const y=v=>base-(scaled(v)-startValue)/span*(base-top);
  const x=i=>left+step*(i+.5);
  const chart=svgNode('svg',{viewBox:'0 0 '+width+' '+height,role:'img','aria-label':axis+', '+display.options[display.selectedIndex].text});
  if(highlight!==undefined){
    const i=rows.findIndex(r=>r.low===highlight);
    if(i>=0) chart.append(svgNode('rect',{x:x(i)-step/2,y:top,width:step,height:base-top,fill:'#eef5ff'}));
  }
  for(let t=0;t<=5;t++){
    const scaledTick=startValue+t*span/5;
    const value=log?10**scaledTick:scaledTick;
    const yy=y(value);
    const tick=mode==='yields'||log?Number(value.toPrecision(2)).toString():value.toFixed(2);
    chart.append(svgNode('line',{x1:left,x2:end,y1:yy,y2:yy,stroke:'#dce4ee'}));
    chart.append(svgNode('text',{x:left-8,y:yy+4,'text-anchor':'end',class:'ticks'},tick));
  }
  if(baseline!==null) chart.append(svgNode('line',{x1:left,x2:end,y1:y(baseline),y2:y(baseline),stroke:'#55677c','stroke-dasharray':'4 4'}));
  rows.forEach((row,i)=>chart.append(svgNode('text',{
    transform:'translate('+x(i)+' '+(base+12)+') rotate(-27)','text-anchor':'end',class:'ticks',
  },binLabel(row))));
  const yLabel={ratios:'Medium / vacuum',gap:'Ratio difference',generators:'C++ / Fortran',yields:'Weighted jet yield per bin',history:'Medium / vacuum'}[mode];
  chart.append(svgNode('text',{transform:'translate(18 '+((top+base)/2)+') rotate(-90)','text-anchor':'middle',class:'axis'},yLabel+(log?' (log)':'')));
  chart.append(svgNode('text',{x:(left+end)/2,y:height-7,'text-anchor':'middle',class:'axis'},axis+' bin'));
  series.forEach(([key,label,color,pilot],j)=>{
    points[j].forEach((p,i)=>{
      if(!p) return;
      const xx=x(i)+(j-(series.length-1)/2)*Math.min(13,step/(series.length+1));
      const yy=y(p.value);
      const group=svgNode('g',{tabindex:'0','aria-label':label+', '+binLabel(rows[i])+', '+format(p)});
      group.append(svgNode('title',{},label+' · '+binLabel(rows[i])+' · '+format(p)));
      if(errors){
        const lower=y(Math.max(low,p.value-p.standard_error)),upper=y(p.value+p.standard_error);
        group.append(svgNode('path',{
          d:'M '+(xx-4)+' '+upper+' h 8 M '+xx+' '+upper+' V '+lower+' m -4 0 h 8',
          fill:'none',stroke:color,'stroke-width':1.4,opacity:pilot ? .6 : 1,
        }));
      }
      group.append(svgNode('circle',{cx:xx,cy:yy,r:pilot?4:4.4,fill:pilot?'white':color,stroke:color,'stroke-width':1.7}));
      chart.append(group);
    });
  });
  const legend=document.createElement('ul');
  legend.className='legend';
  for(const [,label,color,pilot] of series){
    const item=document.createElement('li');
    const dot=document.createElement('span');
    dot.style.borderColor=color;
    dot.style.backgroundColor=pilot?'white':color;
    item.append(dot,label);
    legend.append(item);
  }
  target.replaceChildren(chart,legend);
}

const unitLabels={'Jet pT':'Jet pT (GeV)',ptd:'pTD',mass:'Jet mass (GeV)',girth:'Girth',zg:'zg',groomed_mass:'Groomed mass (GeV)'};
function render(){
  const mode=display.value, observable=menu.value;
  const rows=spectra[observable];
  overlay.disabled=!comparePilot || mode!=='ratios';
  const showPilot=comparePilot && overlay.checked;
  const series=seriesFor(mode,showPilot);
  const axis=unitLabels[observable.split(' | ')[0]];
  const target=document.querySelector('#plot');
  drawPlot(target,rows,series,mode,showErrors.checked,axis,observable==='Jet pT'?60:observable==='ptd | jet pT 40–60 GeV' ? .6 : undefined);
  const secondary=document.querySelector('#secondary');
  secondary.hidden=!comparePilot || observable!=='Jet pT' || mode!=='ratios';
  if(!secondary.hidden){
    drawPlot(document.querySelector('#plot-ptd'),spectra['ptd | jet pT 40–60 GeV'],
      series,mode,showErrors.checked,'pTD',.6);
  }
  document.querySelector('thead tr').replaceChildren(...['Bin',...series.map(([,label])=>label)].map(label=>{
    const th=document.createElement('th');th.textContent=label;return th;
  }));
  document.querySelector('tbody').replaceChildren(...rows.map(row=>{
    const tr=document.createElement('tr');
    for(const value of [binLabel(row),...series.map(([key])=>format(point(row,key,mode)))]) tr.insertCell().textContent=value;
    return tr;
  }));
}
for(const control of [menu,display,overlay,showErrors,scale]) control.addEventListener('change',render);
render();
</script></html>
''')
