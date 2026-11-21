"""Recoil retention must affect colour construction and survive replay."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile

runner, probe = [str(Path(p).resolve()) for p in sys.argv[1:]]
with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    samples = {}
    for label, options in [('default', []), ('on', ['--keep-recoils', 'on']),
                           ('off', ['--keep-recoils', 'off'])]:
        output, replay = root/(label+'.jsonl'), root/label
        subprocess.run([runner, str(output), '1', '2601001', 'medium', str(replay),
                        '--centrality', '0', '10', '--table-energy', 'jewel',
                        '--longitudinal-boost', 'on', *options],
                       check=True, capture_output=True, timeout=60)
        rows = [json.loads(line) for line in output.read_text().splitlines()]
        samples[label] = rows
        assert rows[0]['keep_recoils'] == (label != 'off')
        event = rows[1]
        assert event['type'] == 'event', event
        particles = {p['id']: p for p in event['particles']}
        statuses = [particles[p['id']]['status'] for p in event['hadronization_input']]
        if label == 'off':
            assert not any(s in (3, 13, 22) for s in statuses)
            assert any(p['status'] == 13 for p in particles.values())
            assert all(p['colour'] == p['anticolour'] == 0
                       for p in particles.values() if p['status'] == 13)
            fixture = (replay/'1.input').read_text()
            # The line immediately before the generated-RNG selector configures
            # formation/cuts/hadronize/scatter/storage/write/subtract/capacity.
            lines = fixture.splitlines()
            assert lines[-4].split()[3] == '0'
            result = subprocess.run([probe], input=fixture, text=True,
                                    capture_output=True, check=True, timeout=60)
            records = [list(map(float, line.split())) for line in result.stdout.splitlines()]
            assert records[0][0] == len(particles) and records[0][4] == event['draws']
            for particle, record in zip(event['particles'], records[1:]):
                assert particle['momentum'] == record[5:9]
                assert particle['status'] == record[0]
        else:
            assert 3 in statuses
    assert samples['default'] == samples['on']
    assert samples['off'][1]['draws'] == samples['on'][1]['draws']
    for options in [['--keep-recoils'], ['--keep-recoils', 'false'],
                    ['--keep-recoils', 'on', '--keep-recoils', 'off']]:
        output = root/'invalid.jsonl'
        result = subprocess.run([runner, str(output), '1', '2601001', 'medium', *options],
                                capture_output=True, timeout=10)
        assert result.returncode and not output.exists()
print('Recoil inclusion, colour exclusion, default compatibility and replay passed')
