# Contributing to vkmLog

Thank you for wanting to help. This page is for changing vkmLog itself; a bug or a question goes in
an [issue](https://github.com/V-KMilev/vkmLog/issues).

## Before you write code

Open an issue for anything bigger than a fix, so the shape is agreed before the work is done. Read
the [README](README.md) first: its conventions are promises consumers rely on. A line keeps the form
`parseLine` reads back, and a change that moves it says so and why.

## Build and test

vkmLog builds alone:

```sh
git clone https://github.com/V-KMilev/vkmLog
cd vkmLog
cmake -B build -G Ninja -DVKM_LOG_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

It is developed inside vkmEngine, which pins it at `modules/vkmLog` and builds every library's suite:
a change is tested there too, with `ctest -L vkm_log` and the engine's own suites. The engine's
[CONTRIBUTING.md](https://github.com/V-KMilev/vkmEngine/blob/master/CONTRIBUTING.md#build-and-test)
says how to build it.

## What a pull request needs

A clean build with no warnings in GCC and Clang, a passing suite, a test you saw fail without your
fix, and the README updated in the same commit. A commit message is lowercase `type: summary` under 72
characters, then a few lines on what the change does as a whole and why. One subject per pull request,
in as few commits as tell it.

## How the code looks

As vkmEngine's, listed in
[its CONTRIBUTING.md](https://github.com/V-KMilev/vkmEngine/blob/master/CONTRIBUTING.md#how-the-code-looks).
Match the files around your change.

## The license

vkmLog is source-available under the [vkmEngine License](LICENSE), not an open-source license. By
submitting a contribution you grant the licensor the license its Contributions section describes, and
confirm the work is yours to give. Changed code may be shared only as a contribution here or in a fork
within this repository's GitHub fork network.
