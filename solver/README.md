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

## Running

```bash
./build/release/solver/ev_vrptw            # runs exp1
./build/release/solver/ev_vrptw --exp 0    # re-simulates exp1's saved solutions
./build/release/solver/ev_vrptw --help
```

Inputs are read from `<repo>/data` and outputs written to `<repo>/results`. CMake bakes these
paths in, so the executable works from any directory. To use other locations, pass
`--data DIR` / `--results DIR`, or set `EV_VRPTW_DATA` / `EV_VRPTW_RESULTS`.
All file paths are built in `experiments/ProjectPaths.hpp`.

Cities whose inputs are missing are skipped with a message. Only Bath and Cambridge are
committed; generate the others with `dataprep/`.

| Output | Path (under `results/`) |
|---|---|
| LNS progress per iteration | `searchProgress.csv` |
| Solution (exp1) | `solutions/flatland_<scale>/<benchmark>/route_log_<month>.txt` |
| Detailed simulation | `solutions/flatland_<scale>/<benchmark>/[<City>/]detailed_route_log_<month>[_<day>].csv` |
| Route snapshots (optional) | the `videoDir` passed to `LNS`/`LNSar`: `new/<it>.txt`, `best/<it>.txt` |

## Inputs

| File | Format |
|---|---|
| Instance CSV `data/processed/instances/<City>_<scale>_<benchmark>.csv` | `id,x,y,demand,ready_time,due_time,service_time,scaled_x,scaled_y,lat,lon,elevation` (first row is the depot; read by column position) |
| Short paths `data/processed/short_paths/<benchmark>_short_paths.txt` | One line per pair, `from to: <steps>`; lines starting with `#` are ignored |
| Weather CSV `data/weather/open-meteo/<City>-weather.csv` | Open-Meteo hourly data: `month,day,hour,temperature_2m,dew_point_2m,rain,...` |

## TODO

- [x] Replace hard-coded `C:/Users/...` paths with a data root and an output root
- [ ] Choose the experiment and its parameters from the command line or a config file, not constructors
- [ ] Merge `LNS` and `LNSar`
- [ ] Tidy: remove `using namespace std` from headers and `<bits/stdc++.h>` from `Route.hpp`
- [ ] Add tests (e.g. a small instance with a known route cost)
