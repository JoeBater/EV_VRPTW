# solver

The C++17 EV-VRPTW solver.

## Layout

```
solver/
├── include/        Public headers for the core library
├── src/            Core library sources (built as ev_vrptw_core)
└── experiments/    Experiment drivers and main.cpp (built as ev_vrptw)
```

### Core library (`ev_vrptw_core`)

| Component | Role |
|---|---|
| `Instance` | Loads an instance (customers, demands, time windows, coordinates, elevation), the short-path file and the weather file. Builds the distance and slope matrices and holds the current set of routes. Also writes route and detailed logs. |
| `Route` | One vehicle route. Simulates the route over time (arrival, service and departure times, lateness, loading) and its energy use (state-of-charge per step, using slope, load, speed, acceleration, regen braking, temperature-dependent HVAC and rain). |
| `MST` | Shortest paths between pairs of customers, read from a `*_short_paths.txt` file. Legs are expanded into `SubStep`s. |
| `LNS` | Large Neighbourhood Search: destroy and repair operators, 2-opt, quadrant-based moves. |
| `LNSar` | LNS variant for adaptive routing. Currently a near-copy of `LNS`; due to be merged. |
| `*.hpp` structs | Small value types: `SoCEntry`, `LegResult`, `StepResult`, `MinuteResult`, `SolutionScore`, `StoreData`, `StoreCount`, `Quadrant`. |

### Experiments (`ev_vrptw`)

- `exp0`: re-simulates existing solutions across cities and dates
- `exp1`: runs LNS on chosen benchmarks, then re-evaluates the routes under different cities and weather
- `exp2`: placeholder

`main.cpp` picks which experiment to run. For now an experiment runs from its class constructor.

## Building

From the repository root:

```bash
cmake --preset release      # or: debug
cmake --build --preset release
```

The executable is written to `build/<preset>/solver/ev_vrptw(.exe)`.
`compile_commands.json` is exported for clangd (the root `.clangd` points at `build/release`).

Without presets:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Inputs

| File | Format |
|---|---|
| Instance CSV | `cust_no.,xcoord.,ycoord.,demand,ready_time,due_date,service_time,scaled_x,scaled_y,latitude,longitude,elevation` (first row is the depot) |
| Short paths | One line per pair, `from to: <steps>`; lines starting with `#` are ignored |
| Weather CSV | Open-Meteo hourly data: `month,day,hour,temperature_2m,dew_point_2m,rain,...` |

## TODO

- [ ] Replace hard-coded `C:/Users/...` paths with a data root and an output root (CLI or config)
- [ ] Choose the experiment and its parameters from the command line or a config file, not constructors
- [ ] Merge `LNS` and `LNSar`
- [ ] Tidy: remove `using namespace std` from headers and `<bits/stdc++.h>` from `Route.hpp`
- [ ] Add tests (e.g. a small instance with a known route cost)
