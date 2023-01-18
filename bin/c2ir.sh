#!/bin/bash
#
# usage: c2ir.sh <*.c>...
#
# assumes that the script is in the same directory as the ll2ir executable.
#
# this script uses clang and has only been tested with version 14. the command
# outputs two IR versions, one in LLVM's standard SSA form and one in (mostly)
# non-SSA form. the non-SSA form may still contain phi functions for temporary
# variables introduced during compilation.

# exit when any command fails
set -e

# the location of the ll2ir executable should be in the same directory as this script.
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
LL2IR="$SCRIPT_DIR/ll2ir"

# translate the files
for file in "$@"
do
    # get filename and extension
    filename=$(basename -- "$file")
    extension="${filename##*.}"
    filename="${filename%.*}"

    if [ "$extension" != "c" ]; then
        echo "unknown file extension '$extension'"
        continue
    fi

    # compile to LLVM in file '$filename.ll'.
    clang -c -S -g -fno-discard-value-names -O0 -Xclang -disable-O0-optnone -emit-llvm $file

    # translate preserving SSA
    echo "$filename --> SSA"
    opt -S --mergereturn --mem2reg --instnamer "${filename}.ll" > "${filename}.ssa.ll"
    $LL2IR "${filename}.ssa.ll" > "${filename}.ssa.ir"
    rm "${filename}.ssa.ll"

    # translate without preserving SSA
    echo "$filename --> unSSA"
    opt -S --mergereturn --instnamer "${filename}.ll" > "${filename}.nossa.ll"
    $LL2IR --nossa "${filename}.nossa.ll" > "${filename}.nossa.ir"
    rm "${filename}.nossa.ll"

    rm "${filename}.ll"
done
