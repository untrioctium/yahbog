# yahbog - Yet Another Hobby Boy of Games
Yahbog is just another hobby Game Boy emulator.  While it is still in development, I have these goals:

* `constexpr` as much as possible. Actually pretty easy to do with the relaxing of `constexpr` functions and allocations in the last few standards. The core emulator just shovels bits around with no heap allocations while running, so pretty much all of it qualifies. It also means things go really fast really correctly; I think that's what Jason Turner's talks boil down to anyways...
* Write a cross-platform application with broad compiler support. It should be as easy to build and run on Windows as it is to build WASM, but I'm slowly learning that feature support is not consistent across newer standards. I'd like to keep to a common subset instead of having feature-test macros everywhere.
* Great performance. For example, accessing memory is always constant time thanks to a compile-time generated jump table; no cascading switch statements or anything like that. It probably doesn't need to be that fast, but it's Fun™.
* Eventual GBC support. I'm starting at the basics and working up.

I am not aiming for 100% accuracy (at least for now). The emulator works on an M-Cycle granularity and not T-Cycle; from what I've read, this is enough for the majority of ROMs.

## Building and running
```bash
cmake -B build .
cmake --build build
```
You may need to install system dependencies under Linux. I will note these later.

Currently, only tests exist. To run them:
```bash
pip install requests # if you don't have it
python3 scripts/get_testing_deps.py
./build/src/yahbog-tests/yahbog-tests # Linux
./build/src/yahbog-tests/Release/yahbog-tests.exe # Windows
```

This will run a suite of processor tests, including invidiual instruction sets and various ROMs like Blargg's tests.