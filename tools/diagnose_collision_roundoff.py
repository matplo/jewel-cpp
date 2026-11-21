"""Instrument temporary collision sources without editing the production port.

Requires the existing CMake Unix Makefiles reference build (Release settings
are reused). Diagnostic writes occur immediately before the suspect sqrt.
Use --all-collisions to include positive near-zero squares, which can amplify
roundoff without producing NaNs. Every diagnostic run is checked against the
 uninstrumented stdout.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
from pathlib import Path
import shlex
import subprocess


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def replace_once(text, old, new):
    if text.count(old) != 1:
        raise ValueError('expected unique instrumentation site: ' + old)
    return text.replace(old, new)


def flags(path, prefix):
    for line in path.read_text().splitlines():
        if line.startswith(prefix + ' = '):
            return shlex.split(line.split(' = ', 1)[1])
    raise ValueError('missing build flags: ' + prefix)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path)
    parser.add_argument('fixture', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--all-collisions', action='store_true',
                        help='Record every reshuffled collision, including small positive squares')
    args = parser.parse_args()
    build, fixture, output = (p.resolve() for p in [args.build, args.fixture, args.output])
    root = Path(__file__).resolve().parents[1]
    output.mkdir(exist_ok=False)
    cache = dict(line.split('=', 1) for line in (build/'CMakeCache.txt').read_text().splitlines()
                 if line and not line.startswith(('#', '//')) and '=' in line)
    commands = []

    def execute(command, cwd=build):
        commands.append(command)
        subprocess.run(command, cwd=cwd, check=True, capture_output=True, text=True, timeout=120)

    cpp_source = root/'src/collision.cpp'
    cpp = cpp_source.read_text().replace('#include <limits>', '#include <limits>\n#include <iostream>\n#include <iomanip>')
    site = '  pc={std::sqrt(p32)*pc.px/p3old'
    insertion = ('  if(p32<0||p42<0)std::cerr<<std::setprecision(17)<<"NEGATIVE_COLLISION_SQUARE "'
                 "<<p32<<' '<<p42<<' '<<e3<<' '<<e4<<' '<<m32<<' '<<m42<<' '<<shat<<' '<<t<<' '<<p3old<<'\\n';\n")
    if args.all_collisions:
        insertion = insertion.replace('if(p32<0||p42<0)', '').replace('NEGATIVE_COLLISION_SQUARE', 'COLLISION_SQUARE')
    cpp = replace_once(cpp, site, insertion + site)
    cpp_temp = output/'collision-diagnostic.cpp'
    cpp_temp.write_text(cpp)
    cpp_object = output/'collision-diagnostic.o'
    execute([cache['CMAKE_CXX_COMPILER:FILEPATH'], *flags(build/'CMakeFiles/jewel_core.dir/flags.make', 'CXX_FLAGS'),
             '-I'+str(root/'include'), '-I'+str(root/'src'), '-c', str(cpp_temp), '-o', str(cpp_object)])
    cpp_link = shlex.split((build/'CMakeFiles/jewel-medium-probe.dir/link.txt').read_text())
    cpp_binary = output/'jewel-cpp-diagnostic'
    cpp_link[cpp_link.index('-o')+1] = str(cpp_binary)
    cpp_link.insert(cpp_link.index('libjewel_core.a'), str(cpp_object))
    execute(cpp_link)

    fortran_source = build/'medium-reference.f'
    source = fortran_source.read_text()
    site = '\t   p(n,1) = sqrt(p32)*p(n,1)/p3old'
    insertion = ("      if(p32.lt.0.d0.or.p42.lt.0.d0)then\n"
                 "      write(0,*) 'NEGATIVE_COLLISION_SQUARE',p32,p42,E3new,E4new,\n"
                 "     & m32,m42,shat,t,p3old\n      endif\n")
    if args.all_collisions:
        insertion = insertion.replace('if(p32.lt.0.d0.or.p42.lt.0.d0)then', 'if(.true.)then').replace('NEGATIVE_COLLISION_SQUARE', 'COLLISION_SQUARE')
    source = replace_once(source, site, insertion + site)
    fortran_temp = output/'medium-diagnostic.f'
    fortran_temp.write_text(source)
    fortran_object = output/'medium-diagnostic.o'
    execute([cache['CMAKE_Fortran_COMPILER:FILEPATH'], *flags(build/'CMakeFiles/jewel-medium-reference.dir/flags.make', 'Fortran_FLAGS'),
             '-c', str(fortran_temp), '-o', str(fortran_object)])
    fortran_link = shlex.split((build/'CMakeFiles/jewel-medium-reference.dir/link.txt').read_text())
    fortran_link[fortran_link.index('CMakeFiles/jewel-medium-reference.dir/medium-reference.f.o')] = str(fortran_object)
    fortran_binary = output/'jewel-fortran-diagnostic'
    fortran_link[fortran_link.index('-o')+1] = str(fortran_binary)
    execute(fortran_link)

    def replay(pair):
        label, baseline, diagnostic = pair
        runs = []
        for name, binary in [('baseline', baseline), ('diagnostic', diagnostic)]:
            stdout, stderr = output/(label+'-'+name+'.out'), output/(label+'-'+name+'.err')
            with fixture.open('rb') as inp, stdout.open('wb') as out, stderr.open('wb') as err:
                result = subprocess.run([str(binary)], stdin=inp, stdout=out, stderr=err, timeout=90)
            if result.returncode:
                raise RuntimeError('replay failed: '+str(binary))
            runs.append((stdout, stderr))
        if runs[0][0].read_bytes() != runs[1][0].read_bytes():
            raise AssertionError('diagnostic changed event stdout: '+label)
        marker = 'COLLISION_SQUARE' if args.all_collisions else 'NEGATIVE_COLLISION_SQUARE'
        squares = [list(map(float, line.split()[1:])) for line in runs[1][1].read_text().splitlines()
                   if line.strip().startswith(marker)]
        negative = [row for row in squares if row[0]<0 or row[1]<0]
        return dict(engine=label, negative_collision_squares=negative,
                    recorded_collision_squares=squares,
                    unchanged_stdout_sha256=digest(runs[0][0]),
                    baseline_executable_sha256=digest(baseline),
                    diagnostic_executable_sha256=digest(diagnostic))

    with ThreadPoolExecutor(max_workers=2) as pool:
        results = list(pool.map(replay, [('cpp', build/'jewel-medium-probe', cpp_binary),
                                        ('fortran', build/'jewel-medium-reference', fortran_binary)]))
    report = dict(scope='diagnostic copies; no physics changes; unchanged stdout is not reference parity',
                  all_collisions=args.all_collisions, fixture_sha256=digest(fixture), columns=['p32','p42','e3','e4','m32','m42','shat','transfer_t','p3old'],
                  results=results, commands=commands,
                  sources={str(p):digest(p) for p in [cpp_source,fortran_source,cpp_temp,fortran_temp]})
    (output/'report.json').write_text(json.dumps(report,indent=2,allow_nan=False)+'\n')
    print(json.dumps(results,indent=2,allow_nan=False))


if __name__ == '__main__':
    main()
