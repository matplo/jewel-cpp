"""Native flow option must agree with its independently constructed replay."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile

runner, probe = map(lambda p: str(Path(p).resolve()), sys.argv[1:])
with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    samples = {}
    for label, options in [('default', []), ('off', ['--longitudinal-boost', 'off']),
                           ('on', ['--longitudinal-boost', 'on'])]:
        output, replay = root / (label + '.jsonl'), root / label
        subprocess.run([runner, str(output), '1', '2601001', 'medium', str(replay),
                        '--centrality', '0', '10', '--table-energy', 'jewel', *options],
                       check=True, capture_output=True, text=True, timeout=60)
        rows = [json.loads(line) for line in output.read_text().splitlines()]
        samples[label] = rows
        assert rows[0]['longitudinal_scatterer_boost'] == (label == 'on')
        event = rows[1]
        assert event['type'] == 'event', event
        fixture = (replay / '1.input').read_text()
        lines = fixture.splitlines()
        for tag in ['FCONFIG', 'RCONFIG', 'XCONFIG', 'PCONFIG', 'QCONFIG']:
            assert lines[lines.index(tag) + 1].split()[-1] == str(int(label == 'on'))
        if label == 'on':
            result = subprocess.run([probe], input=fixture, text=True, capture_output=True,
                                    check=True, timeout=60)
            replay_rows = [list(map(float, line.split())) for line in result.stdout.splitlines()]
            header = replay_rows[0]
            assert header[0] == len(event['particles']) and header[4] == event['draws']
            assert header[5:7] == [0, 0]
            for particle, replay_row in zip(event['particles'], replay_rows[1:]):
                assert particle['momentum'] == replay_row[5:9]
    assert samples['default'] == samples['off']
    assert samples['on'][1]['draws'] != samples['off'][1]['draws']
    for options in [['--longitudinal-boost'], ['--longitudinal-boost', 'true'],
                    ['--longitudinal-boost', 'on', '--longitudinal-boost', 'off']]:
        output = root / 'invalid.jsonl'
        result = subprocess.run([runner, str(output), '1', '2601001', 'medium', *options],
                                capture_output=True, timeout=10)
        assert result.returncode != 0 and not output.exists()
print('Flow options, default compatibility and exact native replay passed')
