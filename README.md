# Coyote

Coyote (stylized `coyote`) is a **work-in-progress** implementation of the [Jackal](https://github.com/xrarch/newsdk) programming language by [hyenasky](https://github.com/hyenasky).
Coyote focuses on lower memory usage, faster compile times, and higher quality code generation compared to the XR/SDK implementation.

Coyote also introduces various backwards-compatible changes to Jackal for ergonomics (pointers to scalar stack variables, implicit `IN`, `ALIGNOF` and `ALIGNOFVALUE`, etc.) and optimization (`NOALIAS` modifier, etc.). 
To discourage use of these features, use `--xrsdk` and add `--error-on-warn` to enforce it.

This repository is also home to Iron, a lightweight but powerful code optimization and generation library. Iron can (and should!) be built as a standalone library for use in other projects.

## Building

You will need Make, a C compiler with support for C23 (with some GNU extensions), and a linker (`gcc` is the default for both of these).

To build from scratch:
```sh
make clean coyote       # for Coyote
make clean libiron      # for Iron as a library
```

To build incrementally, after the initial `clean`
```sh
make coyote             # for Coyote
make libiron            # for Iron as a library
```
