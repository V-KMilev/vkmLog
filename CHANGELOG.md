# Changelog

Notable changes to vkmLog, newest first. Each release is tagged `vX.Y.Z`.

## 0.0.6

- **The console is flushed after every line,** so a program whose output is piped - to a log
  collector, a service manager - shows each line as it is written, not when a buffer fills.

## 0.0.5

The library's names are in `Vkm::Log`.

- **`Logger` and `LogLevel` are `Vkm::Log::Logger` and `Vkm::Log::LogLevel`,** and the assert's
  failure handler, once `__assert_fail` at global scope beside glibc's own, is
  `Vkm::Log::assertFail`. Code that names them qualifies them; the `LOG_*` and `VKM_ASSERT`
  macros are used as before.

## 0.0.4

- **The target is `vkm_log`,** and so is the library file it builds, so an installed prefix
  says whose it is. A consumer links `vkm_log`; includes are unchanged.

## 0.0.3

- **An installed SDK can carry it:** the include path is the source tree in a build and the
  prefix's `include/` once installed, so a target that exports one linking it is accepted.

## 0.0.2

The first release: a thread-safe logger and an assert, as a shared library.

- **Seven levels,** `TRACE` to `FATAL`, each line written to the console and to one file, below
  the level `init` set written nowhere.
- **A line says whose it is:** a translation unit defines `VKM_LOG_SUFFIX` for its own suffix in
  place of the one `init` was given, and `VKM_LOG_CATEGORY` for a `[CATEGORY]` tag between the
  suffix and the level; the `LOG_*_C` macros name a category for one call.
- **`VKM_ASSERT(condition[, format, ...])`** logs a failed condition with a printf-style message
  and aborts; it compiles to nothing where `NDEBUG` is defined.
