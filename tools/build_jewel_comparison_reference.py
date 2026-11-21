"""Build isolated JEWEL 2.6.0 reference with documented compiler/logging fixes."""
import argparse, hashlib, json, shutil, subprocess, sys
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('source',type=Path);p.add_argument('output',type=Path)
p.add_argument('--lhapdf-prefix',type=Path,required=True)
a=p.parse_args();source=a.source.resolve();out=a.output.resolve()
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
expected={'jewel-2.6.0.f.stock':'78b39f19513c3633c00d45b22f0803c0873a93d98a536d66efc9277255bf5b81','medium-simple.f.stock':'a5f5a16d984cf84d28c2c789f72557a64b85cb6ae51bc9f395b1f2a7f4e2bd38','pythia6425mod-lhapdf6.f':'9fcfed664e4ac34462aa63679f2dd3e935c9f6f63f7cd6a785b73ccb82740ed9'}
for name,digest in expected.items():
 if sha(source/name)!=digest:raise ValueError('unexpected reference source: '+name)
s=(source/'jewel-2.6.0.f.stock').read_text()
changes=[('TAUEST,QSUMVECOLD(4),ZDUM,WEIGHT,','TAUEST,QSUMVECOLD(4),ZDUM,'),
 ("FORMAT(I6,' (',I6,',',I6')    ['3I6,' ]  {',I5,I3,' } ',2E14.6,l2)","FORMAT(I6,' (',I6,',',I6,')    [',3I6,' ]  {',I5,I3,' } ',2E14.6,l2)")]
for old,new in changes:
 if s.count(old)!=1:raise ValueError('normalization must match exactly once')
 s=s.replace(old,new)
out.mkdir(exist_ok=False)
(out/'jewel-2.6.0.f').write_text(s)
for name in ['medium-simple.f.stock','pythia6425mod-lhapdf6.f','meix.f','subtraction.C']:
 shutil.copyfile(source/name,out/name.removesuffix('.stock'))
commands=[['gfortran','-O2','-std=legacy','-ffixed-line-length-none','-c',name] for name in ['jewel-2.6.0.f','medium-simple.f','pythia6425mod-lhapdf6.f','meix.f']]
commands += [['c++','-c','subtraction.C'],['gfortran','-o','jewel-reference-simple','jewel-2.6.0.o','medium-simple.o','pythia6425mod-lhapdf6.o','meix.o','subtraction.o','-L'+str(a.lhapdf_prefix.resolve()/'lib'),'-lLHAPDF','-lc++' if sys.platform=='darwin' else '-lstdc++']]
for command in commands:subprocess.run(command,cwd=out,check=True)
manifest=dict(scope='Stock JEWEL 2.6.0 physics with duplicate declaration and diagnostic FORMAT fixes only',changes=[dict(old=x,new=y) for x,y in changes],commands=commands,
 sources={name:sha(source/name) for name in [*expected,'meix.f','subtraction.C']},artifacts={name:sha(out/name) for name in ['jewel-2.6.0.f','medium-simple.f','jewel-reference-simple']},builder_sha256=sha(Path(__file__)))
(out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(out/'jewel-reference-simple')
