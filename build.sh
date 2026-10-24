#!/bin/sh
# Build DinoSaw from your own copy of the game:
#     ./build.sh dinosaw.zip     (Acorn Arcade's 2006 re-release: DinoSaw/, with RISC OS filetypes)
#     ./build.sh DIRECTORY       (or the same files, e.g. an emulator's HostFS copy)
# then: cd recomp && ./dinosaw
# The steps are recipe.json's "build", run by SuperSpider's tools/weave.py.
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
SUPERSPIDER="${SUPERSPIDER:-$HOME/stuff/superspider}"
[ $# -gt 0 ] || { echo "usage: $0 dinosaw.zip | DIRECTORY" >&2; exit 2; }
(cd "$SUPERSPIDER" && dune build)
WEAVE_RECIPES="$HERE" exec python3 -I "$SUPERSPIDER/tools/weave.py" build "$@"
