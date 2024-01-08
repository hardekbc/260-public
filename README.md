# [for Winter 2023; now out of date]

# CS 260 Program Analysis

This repo contains the public infrastructure for CS 260 assignments for implementing program analyses. The infrastructure assumes that the following packages are installed and available on your system:

- clang (version 14)

- bazel

- glog

- googletest

# Contents

- `analysis`: The directory where your analysis implementations for the assignments will go. Currently contains an empty BUILD file with example templates for library and test build rules.

- `bin`: Contains some useful scripts.

    + `c2ir.sh`: Takes a list of C filenames as input and outputs our simplified IR.

    + `ll2ir`: Takes an LLVM bitcode file as input and outputs our simplified IR (used by the `c2ir.sh` script, you shouldn't need to use it directly).

    + `refresh_compiler_commands.sh`: A utility to create a `compiler_commands.json` file for any utility that needs one (this is entirely optional, some IDEs and tools need this file and others don't).

- `example-c-programs`: A set of C programs that are in our restricted subset of the C language. To translate them into our simplified IR:

    ```
    cd example-c-programs
    ../bin/c2ir.sh *.c
    ```

- `ir`: Contains the library defining a datastructure for holding an IR program, plus some additional useful libraries. To build these libraries:

    ```
    bazel build ir:all
    ```

    To run tests on these libraries:

    ```
    bazel test ir:all
    ```

    Note that building vs testing uses somewhat different compiler flags (testing uses the debugging flags and the address sanitizer for checking for memory errors, for example). See `.bazelrc` for the exact compiler commands being used.

- `util`: Contains some useful utilities that can be used by other libraries. As a general rule, all libraries should probably include `standard_includes.h`.
