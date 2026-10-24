DinoSaw (Tom Cooper, 1993; freeware re-release by Acorn Arcade, 2006) - static recompilation
==========================================================================================

The input is Acorn Arcade's January 2006 re-release, `dinosaw.zip`: a zip of
DinoSaw/ (!DinoSaw, !DinoKeys, !MaxMem, Instruct) with the files' RISC OS
types in its extra fields. It is "version 1.1, September '94", with someone's
RISC OS 4 fixes (a newer TrackerModule, an SCache step in !Run).

A recipe: this repository holds no game data. It rebuilds the game from
your own copy, recognised by content (recipe.json):

    ./build.sh dinosaw.zip
    cd recomp && ./dinosaw

build.sh runs recipe.json's steps with SuperSpider's tools/weave.py
(~/stuff/superspider, or $SUPERSPIDER; $SS below): it unpacks the zip to
extracted/zip and !DinoSaw to game/ (libroger's appdir), then analyses,
translates and links. Needs python3, OCaml/dune, gcc and SDL3.

    tools/images.py     the translated images; `analyse` runs SuperSpider
                        on each, `gen` writes recomp/gen/<image>/
    analysis/           SuperSpider's output
    basic/              the BASIC programs, detokenised (make basic in recomp/)
    recomp/             make regen (analysis + gen), make, ./dinosaw

Only !DinoSaw is used: !DinoKeys (a BASIC key redefiner, writing
MiscData.Keys) and !MaxMem (frees memory on a 1MB machine) are not.

Running
-------

    cd recomp && make && ./dinosaw [--no-sound] [--scale=N] [--fullscreen]
    ./dinosaw --level=N        a new game starts at level N (1-11)
    ./dinosaw --save=DIR       where the high score table the game saves goes (default savedata/)
    ./dinosaw --trace-gaps     interpret untranslated code instead of stopping (none found so far)
    ./dinosaw --limit=N        headless, deterministic: Space, then walking right and a jump

Keys (Instruct): title page - Space start, Escape quit, Print 1 or 2
players. In the game - player 1 Z left, X right, C jump; player 2 keypad 1,
2, 3; Escape back to the title. A controller: left/right walk, up, A or B
jump, start Space. `--script` key numbers are negated INKEY numbers: Space
99, Z 98, X 67, C 83, Escape 113, Print 33.

What runs
---------

!Run (`*FX 200 3`, WimpSlot 640k), in order, and what the recompilation
does with each step:

- Code.BertyCorp (BASIC): the BertyCorp and Five Star Marketing pictures,
  each faded in, held for 2.55s or a key, and faded out; then VOICES 4 and
  MODE 13 with both banks cleared. C in recomp/game.c (the pictures in
  interactive runs only). It *Loads Code.Crunchy, a 156-byte RLE decoder,
  and CALLs it on each picture: Gyrinus II's decoder (that game assembles
  it in BASIC), so game.c's decode() is the same loop in C.
- Code.AdCode: an absolute loaded at &551C0 and entered at &55EB8:
  one of four adverts for Tom Cooper's other games (InfoScr1-4: Cycloids,
  2067 BC, ...) at random until Space or about 20s, then the loading
  screen. It returns to its caller. Translated (gen/adcode); game.c loads
  it and calls it with its translation selected (interactive runs only).
- SCache (BASIC): `SYS &20065,4,3,1`, XOS_ScreenMode 4, the Risc PC's
  screen cache; an error, ignored, on an Archimedes. Not run.
- Code.MCode: the game, translated (gen/mcode). An absolute loaded at
  &89950 and entered at &A1DFC. It installs its own sound effect voice
  generators (Sound_InstallVoice), *RMLoads Music.PlayerMod
  (`RmEnsure TrackerModule 1.04 RmLoad ...`) and plays Music.Tracker4 on
  the title page and Music.Tracker2 in the game with *PlayMod - the
  latter only on a fast machine: at start-up (&A1CF0) it times a count to
  &100000 and keeps the music on in the game (+&2E4) if that took under
  30cs, i.e. on an ARM3. Interactive runs are as fast as the host, so
  they have it; headless runs (2 MIPS of emulated time, an ARM2) and
  `--mips=4.5` don't. It *Loads
  and *Saves MiscData.AllTimeHi (the high score table, XORed with
  &76341277).
- Music.PlayerMod: TrackerModule 4.09 (17 Dec 1996), a replacement for the
  1993 release's player. A relocatable module, translated for &1800000
  (gen/tracker) and loaded when MCode asks for it. It drives the sound
  system with its own channel handler (Sound_Configure).
- Code.EndCredits (BASIC): the credits picture for 45s or a key. C in
  game.c, run when MCode returns (interactive runs only).

Levels: MCode keeps its workspace at &89450 (R12). +&180 is the level,
1-11, which names the file loaded (Level0.LevelAB to LevelAL). After the
11th the game is complete ("Congrats... You've completed DinoSaw"). Files
LevelAM-AP are never loaded (Instruct's "fifteen levels" counts them).
+&320 and +&3A0 are the two players' scores. Setting +&184 (&895D4) to 1
completes the level being played; that and a poked score are how the
ending and high score entry were checked (`--script=...,poke:0x895D4:1`).

What was checked, interactively (SDL's offscreen driver and `--script`
screenshots) and headless: the start-up pictures and adverts, the title
page and high score table, every level (`--level=1..11` with random play
and `--trace-gaps`: no untranslated code), dying and game over, the
ending, high score name entry and the table's save and reload, Escape out
of the game and out of the program into EndCredits, the title music and the
effects (a --wav recording of a headless run, so without the in-game music). Two-player mode ran headless without gaps,
but wasn't checked on screen.

Known differences and open questions
------------------------------------

- Headless screenshots are often black or half-drawn: as for Gyrinus II,
  libroger answers VDU variable 149 (DisplayBank's address) with the write
  bank in headless runs. Headless runs are for comparing builds; look at
  interactive ones.
- The music's pitch and tempo haven't been compared with the original.

libroger and SuperSpider changes this game needed (both general):

- libroger: Sound_InstallVoice no longer calls the voice generator's +24
  Install entry. RISC OS never calls it (RISC OS Open's documentation:
  "not supported and simply returns"). MCode's voice blocks have their
  name offset at +24, which runs on into MCode's own install routine,
  which calls Sound_InstallVoice again: an endless recursion.
- SuperSpider: a function returning through its entry LR with the PSR
  restored (ldm ...,{...,pc}^ or movs pc,lr) leaves the caller's flags as
  they were at the bl. TrackerModule calls `bleq f ; blne g` with a return
  address passed in r1, and f returns so: without this the analysis took
  blne g as possibly run after f (with r1 changed), and two returns
  stayed unresolved.
