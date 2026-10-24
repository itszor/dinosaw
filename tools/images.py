#!/usr/bin/env python3
"""The translated images of DinoSaw and the steps that build them.

    images.py analyse   SuperSpider (superspider analyse --full) on every image:
                        analysis/<name>.json; analysis/<name>.roots, if there,
                        adds entry points
    images.py roots LOG...
                        merges the gap entries of --trace-gaps runs (libroger's
                        untranslated.log) into analysis/<name>.roots
    images.py gen       superspider gen for every image -> recomp/gen/<name>/

Images:
  mcode      Code.MCode: the game. An absolute file loaded at &89950 and
             entered at &A1DFC (its execution address).
  adcode     Code.AdCode: the information screens !Run shows before the game
             (one of InfoScr1-4 at random, then Loading). An absolute loaded
             at &551C0 and entered at &55EB8; it returns to its caller.
  tracker    Music.PlayerMod: TrackerModule 4.09, the music player MCode
             *RMLoads (RMEnsure TrackerModule 1.04) and drives with *PlayMod,
             at a fixed RMA address.
"""
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
TOP = os.path.dirname(HERE)
EXT = os.path.join(TOP, 'game')                 # !DinoSaw (recipe.json)
SS = os.environ.get('SUPERSPIDER', os.path.expanduser('~/stuff/superspider'))

TRACKER_BASE = 0x1800000


def find(d, leaf):
    for f in os.listdir(os.path.join(EXT, d)):
        if f.split(',')[0].lower() == leaf.lower():
            return d + '/' + f
    raise SystemExit('not found: %s/%s' % (d, leaf))


def images():
    """(name, path relative to game/, base, exec or None, is module)"""
    return [('mcode', find('!DinoSaw/Code', 'MCode'), 0x89950, 0xA1DFC, False),
            ('adcode', find('!DinoSaw/Code', 'AdCode'), 0x551C0, 0x55EB8, False),
            ('tracker', find('!DinoSaw/Music', 'PlayerMod'), TRACKER_BASE, None, True)]


def run(cmd):
    print('+', ' '.join(cmd), flush=True)
    subprocess.run(cmd, check=True)


def analyse():
    subprocess.run(['dune', 'build'], cwd=SS, check=True)
    exe = os.path.join(SS, '_build/default/bin/main.exe')
    an = os.path.join(TOP, 'analysis')
    os.makedirs(an, exist_ok=True)
    for name, path, base, exe_addr, module in images():
        img = os.path.join(EXT, path)
        cmd = [exe, 'analyse', img, '%x' % base, '--full', '-o', os.path.join(an, name + '.json')]
        rf = os.path.join(an, name + '.roots')
        if os.path.exists(rf):
            cmd += ['--roots', rf]
        cmd += ['--module'] if module else ['--exec', '%x' % exe_addr]
        run(cmd)


def roots(logs):
    """Merges the gap entries the --trace-gaps runs logged in LOGS (libroger's
    untranslated.log) into analysis/<name>.roots, for the next analyse."""
    an = os.path.join(TOP, 'analysis')
    for name, path, base, exe_addr, module in images():
        run([sys.executable, os.path.join(SS, 'tools/gap_roots.py'), os.path.join(EXT, path),
             '%x' % base, os.path.join(an, name + '.roots')] + logs)


def gen():
    exe = os.path.join(SS, '_build/default/bin/main.exe')
    rc = os.path.join(TOP, 'recomp')
    an = os.path.join(TOP, 'analysis')
    for k, (name, path, base, exe_addr, module) in enumerate(images()):
        tab = 'recomp' if name == 'mcode' else 'recomp_' + name
        prefix = {'mcode': 'F', 'adcode': 'A', 'tracker': 'M'}[name]
        run([exe, 'gen', os.path.join(EXT, path), '%x' % base, os.path.join(an, name + '.json'),
             '--c', os.path.join(rc, 'gen', name), '--prefix=' + prefix, '--tabprefix=' + tab,
             '--files=%d' % (4 if name == 'mcode' else 1)]
            + (['--covtag=%s' % chr(ord('A') + k)] if name != 'mcode' else []))


if __name__ == '__main__':
    if sys.argv[1] == 'roots':
        roots(sys.argv[2:] or ['untranslated.log'])
    else:
        {'analyse': analyse, 'gen': gen}[sys.argv[1]]()
