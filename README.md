# Harris Lab

Harris Lab is a native C++ desktop application for exploring the standard two-dimensional contact process. It displays a square periodic lattice in real time and uses the exact continuous-time Harris graphical construction rather than a synchronous cellular-automaton approximation.

![C++20](https://img.shields.io/badge/C%2B%2B-20-6de09a) ![CMake](https://img.shields.io/badge/CMake-3.20%2B-6de09a)

## Model

Each infected site:

- recovers at rate `1`; and
- attempts to infect each of its four nearest neighbors at rate `lambda`.

The lattice has periodic boundary conditions. The engine draws exponential holding times and simulates recovery marks and directed infection arrows exactly. Attempts aimed at already infected sites are valid null events, as they are in the graphical construction.

> The critical value for the infinite square lattice is close to `lambda = 0.4122` under this **per-neighbor** convention. A larger default (`1.65`) is intentionally used so a single seed usually produces visible activity. Different references sometimes divide the infection parameter among neighbors, so check conventions when comparing results.

## Features

- Start, pause, continue, and reset controls
- Configurable per-neighbor infection rate `lambda`
- Square grids from `8 x 8` through `2048 x 2048`
- Single-site, fully infected, or random-density initial states
- Logarithmic observation-speed control from `0.1x` to `100x`
- Live simulated time, infected count, extinction detection, and event-cap feedback
- Efficient constant-time infection and recovery updates

For interactive use, `32 x 32` through `256 x 256` is a good starting range. Grids around `1000 x 1000` or larger require substantially more memory bandwidth to redraw and may feel slow, depending on the machine.

## Build on Windows

Install [Git](https://git-scm.com/), [CMake](https://cmake.org/download/), and Visual Studio 2022 with the **Desktop development with C++** workload. Then open PowerShell in the repository:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

CMake downloads raylib 5.5 during its first configuration. The executable will be:

```text
build\Release\harris_lab.exe
```

Double-click `harris_lab.exe` or run it from PowerShell.

## Build on Linux or macOS

Install a C++20 compiler and CMake, then run:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/harris_lab
```

On Linux, raylib's usual X11 development packages may be required. On Debian/Ubuntu these commonly include `libx11-dev`, `libxrandr-dev`, `libxinerama-dev`, `libxcursor-dev`, and `libxi-dev`.

## Tests

The stochastic engine has no graphics dependency, so tests can be built even when raylib or a display server is unavailable:

```bash
cmake -S . -B build-tests -DHARRIS_BUILD_GUI=OFF
cmake --build build-tests --parallel
ctest --test-dir build-tests --output-on-failure
```

## Project layout

```text
include/contact_process.hpp       Public simulation API
src/contact_process.cpp           Continuous-time stochastic engine
src/main.cpp                      Desktop interface and lattice renderer
tests/contact_process_tests.cpp   Deterministic engine checks
CMakeLists.txt                    Cross-platform build configuration
```

## Controls and interpretation

Changing lambda, grid size, or initial condition marks the configuration for reset. Pressing **Start** applies it. **Pause** preserves the current realization, and **Start** then continues it unless a setting changed. **Reset** immediately creates a fresh realization with the current settings.

The speed slider changes simulated time requested per wall-clock second; it does not alter transition probabilities. To keep the interface responsive, one rendered frame is capped at 250,000 stochastic events. The status line reports when that safeguard is reached.
