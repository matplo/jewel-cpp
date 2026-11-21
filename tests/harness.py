import hashlib, json, subprocess, sys, tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as folder:
    tmp=Path(folder)
    source=tmp/'jewel-2.6.0.f'
    source.write_text('not the frozen source')
    bad=subprocess.run([sys.executable,str(root/'reference/extract.py'),str(tmp),str(tmp/'out.f')],capture_output=True,text=True)
    assert bad.returncode != 0 and 'Refusing' in bad.stderr
    assert not (tmp/'out.f').exists()
    cmd=[sys.executable,str(root/'reference/freeze.py'),'--variant','stock','--output',str(tmp/'baseline'),'--file',str(source)]
    subprocess.run(cmd,check=True,capture_output=True)
    record=json.loads((tmp/'baseline/manifest.json').read_text())
    assert record['variant']=='stock'
    item=record['files'][0]
    data=(tmp/'baseline'/item['snapshot']).read_bytes()
    assert data==source.read_bytes() and hashlib.sha256(data).hexdigest()==item['sha256']
    assert subprocess.run(cmd,capture_output=True).returncode != 0
print('PASS: source rejection, snapshot integrity, overwrite protection')
