#!/bin/bash
#
# run this script from the 260-public/ directory (i.e., the same location the WORKSPACE file is located) to create a compiler_commands.json file.
bazel run @hedron_compile_commands//:refresh_all
bazel clean
