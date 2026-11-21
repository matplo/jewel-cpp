"""Render observable-selectable distributions and native/Fortran ratios."""
import argparse,json
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('input',type=Path);p.add_argument('output',type=Path)
a=p.parse_args();data=json.loads(a.input.read_text())
if data['schema']!='jewel-jet-comparison-v1':raise ValueError('unsupported comparison schema')
html=Path(__file__).with_name('jet_comparison.html').read_text()
a.output.write_text(html.replace('__DATA__',json.dumps(data,allow_nan=False).replace('<','\\u003c')))
print(a.output)
