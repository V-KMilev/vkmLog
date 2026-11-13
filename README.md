# vkmLog

A thread-safe logger and an assert. Every line goes to the console and to one file, in one form that
`parseLine` reads back:

```
[2026-01-02 03:04:05.006] [SUFFIX] [CATEGORY] [LEVEL] message
```

## Conventions

- Seven levels, `Level::Trace` to `Level::Fatal`; a line below the level `init` set is written nowhere.
- A line says whose it is. The suffix is a library's or program's name: a library defines
  `VKM_LOG_SUFFIX` once for its whole target (`target_compile_definitions`), never per file; left
  undefined, a line carries the suffix `init` was given. The category is a file's subsystem: a file
  defines `VKM_LOG_CATEGORY` on its first line, before any include; left undefined, the line has none.
- Formats are printf's, and GCC and Clang check each call's arguments against its format under
  `-Wformat`, which `-Wall` turns on.
- `VKM_ASSERT(condition[, format, ...])` keys on `NDEBUG` in the file that expands it: compiled out
  where it is defined (a `Release`, `RelWithDebInfo` or `MinSizeRel` build), and otherwise a failed
  condition logs one `ERROR` line, with the caller's suffix and category, and stops the program
  (`stopProgram`). Before `init` the line goes to stderr.
- The console is flushed on every line, so a program whose output is piped shows each line at once.
- A file that will not open or will not take a line is said on the console, and the lines go on there.

## A program's log

```cpp
#include "vkm_log/logger.h"

Vkm::Log::Logger::init("logs/run.log", "MY-PROGRAM", Vkm::Log::Level::Info);
LOG_INFO("%d files loaded from %s", count, folder.c_str());
LOG_WARNING_C("NET", "a category for this line alone");
```

## Requirements

CMake 3.25 or above and a C++17 compiler.

## Use

Add it as a submodule and `add_subdirectory` it; it requires no other target.

```
git submodule add https://github.com/V-KMilev/vkmLog.git modules/vkmLog
```

The library is `vkm_log`, shared, as the logger is one per process and a consumer may link it into
more than one library of its own. Headers are included by the library's folder:
`#include "vkm_log/logger.h"`; everything is in `Vkm::Log`, but the macros.

| Option                | Default | What                                                                    |
|-----------------------|---------|-------------------------------------------------------------------------|
| `VKM_LOG_BUILD_TESTS` | `OFF`   | Builds `vkm_log_tests`: each test a CTest of its own, labeled `vkm_log` |

## What it holds

| File               | What                                                                       |
|--------------------|----------------------------------------------------------------------------|
| `vkm_log/logger.h` | `Logger`, `Level`, `levelName`, `Line` and `parseLine`; the `LOG_*` macros |
| `vkm_log/assert.h` | `VKM_ASSERT`, `assertFail`, `stopProgram`                                  |

## Tests

`tests/` holds the library's own suite, built with `VKM_LOG_BUILD_TESTS`: each test is
`TEST("what works")` in `tests/<topic>.cpp`, a CTest named `vkm_log: <what works>` and run in a
process of its own, so `ctest -L vkm_log` runs them in the tree of a consumer that calls
`enable_testing()`, with `-C <config>` under a multi-config generator. Beside them, a CTest that
compiles a call whose arguments do not fit its format and expects the compiler to refuse it.

## Version

0.0.6, set in `CMakeLists.txt`'s `project()`. Each release is tagged `vX.Y.Z` on `master` and listed
in [CHANGELOG.md](CHANGELOG.md); work lands on `vkm/dev` first.

## In vkmEngine

The engine pins it at `modules/vkmLog` and every engine binary links it. A change is a commit here,
then an engine commit that moves the pin.

## License

vkmLog is source-available under the [vkmEngine License](LICENSE), the engine's own, which holds for
each vkm library used alone. [CONTRIBUTING.md](CONTRIBUTING.md) says how a change is shared.
