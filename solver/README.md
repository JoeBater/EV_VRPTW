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
| `Route` | One vehicle route. Simulates the route over time (arrival, service and departure times, lateness, loading) and its energy use per minute (see [Energy model](#energy-model)). |
| `MST` | Shortest paths between pairs of customers, read from a `*_short_paths.txt` file. Legs are expanded into `SubStep`s. |
| `LNS` | Large Neighbourhood Search: destroy and repair operators, 2-opt, quadrant-based moves. |
| `*.hpp` structs | Small value types: `SoCEntry`, `LegResult`, `StepResult`, `MinuteResult`, `SolutionScore`, `StoreData`, `StoreCount`, `Quadrant`. |

### Experiments (`ev_vrptw`)

- `exp0`: the **flatland baseline**, the closest thing to the academic VRPTW. Solves every
  benchmark with LNS on flat terrain at a constant 20 °C with no rain. The site layout is the
  same in every city, so it uses the Cambridge inputs, flattened. Legs are driven along the
  minimum spanning tree (MST), the network all later experiments use.
- `exp0a`: the same, on the **complete graph** (every leg driven directly, Euclidean, as in
  Solomon), to measure how much moving to the MST alone changes the solutions. Each solution is
  also re-simulated on the flat MST (the `mst_*` summary columns). Terrain is never put on the
  complete graph.
- `exp1`: re-simulates each exp0 baseline, without re-optimising, in every city, to separate
  what terrain and weather each do to it:
  - `1a` the city's terrain, flatland weather (one run per city)
  - `1b` the city's weather on every day of the year, flat terrain
  - `1c` terrain and weather, every day of the year

  Each writes one row per city and day: lateness, depleted routes (charge below 0 at any
  minute), lowest and mean final charge, and `feasible` (nothing late, nothing depleted). A
  `flatland` row re-simulates the baseline itself as the reference.
- `exp2`: placeholder

`main.cpp` picks which experiment to run. For now an experiment runs from its class constructor.
The benchmarks, their fleets and the cities are listed in `experiments/Benchmarks.hpp`. Each
benchmark is solved with Solomon's capacity (200, or 1000 for r2/rc2), starting from its best-known
vehicle count. The LNS keeps every route non-empty, so the fleet size is fixed per run; if the
flatland solution has late stops or overload, exp0 adds a vehicle and solves again (up to 5
extra), so every baseline is clean. Matching Solomon's optimum is not the aim.

exp0 and exp0a warm-start from the published solutions in `results/solomon_solutions/<benchmark>.txt`
(SINTEF format, `Route 1 : 5 3 7 ...`; SINTEF customer k is solver index k, depot 0). The
first attempt uses the published routes and fleet; the LNS only accepts improvements, so a
baseline is never worse than its start. Extra-vehicle attempts start from a random assignment.
The summary's `start` column says which (`solomon` or `random`). Every published solution is
feasible in this model on the flat complete graph, at exactly Solomon's distance × 50 m,
because travel here is never slower than Solomon's one unit per minute.

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
./build/release/solver/ev_vrptw                  # exp0: solve all flatland baselines (~6 min each)
./build/release/solver/ev_vrptw --bench r105     # just one (repeatable); --iterations N to change the LNS budget
./build/release/solver/ev_vrptw --exp 0a         # the same on the complete graph
./build/release/solver/ev_vrptw --exp 1          # exp1a, 1b and 1c (~2 min in all); or --exp 1a|1b|1c
./build/release/solver/ev_vrptw --exp 1c --detailed   # also write per-minute route logs (~100 MB per city and benchmark)
./build/release/solver/ev_vrptw --year 2023   # weather year (default 2024; 2022–2025 available)
./build/release/solver/ev_vrptw --exp 0 --no-regen   # regen braking off; logs go to <City>_noregen/
./build/release/solver/ev_vrptw --exp 0 --start-hour 8   # minute 0 = 08:00 for the weather (default midnight); logs go to <City>_start08/
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
| Baseline summary (exp0), one row per fleet tried; `clean = 1` marks the baseline | `solutions/flatland_<scale>/baseline_summary.csv` |
| Complete-graph baselines (exp0a), same layout plus `mst_*` columns | `solutions/complete_<scale>/` |
| Baseline solution (exp0) | `solutions/flatland_<scale>/<benchmark>/route_log.txt` |
| Baseline flatland simulation and LNS progress (exp0) | `solutions/flatland_<scale>/<benchmark>/detailed_route_log.csv`, `searchProgress.csv` |
| Re-simulation summary (exp1a/b/c), one row per city and day | `solutions/flatland_<scale>/<benchmark>/exp1<a\|b\|c>[_noregen][_startHH].csv` |
| Re-simulation per-minute logs (`--detailed`) | `solutions/flatland_<scale>/<benchmark>/<City>[_noregen][_startHH]_<mode>/detailed_route_log[_<month>_<day>].csv` |
| Route snapshots (optional) | the `videoDir` passed to `LNS`: `new/<it>.txt`, `best/<it>.txt` |

## Inputs

| File | Format |
|---|---|
| Instance CSV `data/processed/instances/<City>_<scale>_<benchmark>.csv` | `id,x,y,demand,ready_time,due_time,service_time,scaled_x,scaled_y,lat,lon,elevation` (first row is the depot; read by column position) |
| Short paths `data/processed/short_paths/<benchmark>_short_paths.txt` | One line per pair, `from to: <steps>`; lines starting with `#` are ignored |
| Weather CSV `data/weather/open-meteo/<year>/<City>-weather-<year>.csv` | Open-Meteo hourly data: `month,day,hour,temperature_2m,dew_point_2m,rain,...` |

## TODO

- [x] Replace hard-coded `C:/Users/...` paths with a data root and an output root
- [ ] Choose the experiment and its parameters from the command line or a config file, not constructors
- [ ] Tidy: remove `using namespace std` from headers and `<bits/stdc++.h>` from `Route.hpp`
- [ ] Add tests (e.g. a small instance with a known route cost)
- [ ] **Strengthen the LNS before exp2.** It is pure descent (only improvements are accepted).
  From a random start on the flat complete graph it finished 25–280% longer than the published
  solutions (c107: 159 km vs 41 km), and on the MST it lost to the published routes on 12 of 16
  benchmarks. exp2 re-optimises with no published start, so gains there would mix real
  terrain/weather effects with better search. Candidates: simulated-annealing or
  record-to-record acceptance, route-minimisation moves, regret insertion. The published
  solutions are the yardstick.

## Energy model

Each leg is walked as a chain of terrain segments: the ~100 m profile of every MST edge it
uses (`data/processed/edge_profiles/`), or one uniform slope per edge from the site
elevations when a city has no profile file. Time and energy follow the distance actually
covered; only the arrival at a site is rounded up to a whole minute.

On each piece of road of length *d* and grade *g* (clamped to ±25%, because the Copernicus
DEM is a surface model that includes buildings and trees):

- **Level-road energy**: `0.4 kWh/km × d`, scaled by temperature, speed band and load.
- **Gravity** at the wheel: `m·g·(g·d)`, with *m* = 2,500 kg van + 5 kg per unit of load.
  Climbing draws it from the battery through a 0.9-efficient drivetrain. Descending first
  offsets the level-road losses; any surplus is regenerated at 0.6 efficiency (halved above
  2 mm/h of rain), or lost to the friction brakes with `--no-regen`.
- **Acceleration**: `½·m·v²` at the start of each MST edge after a stop.
- **Parked**: HVAC draw, by temperature band, while waiting or serving.

Speed comes from the edge length band (25/35/55 km/h), slowed by gradient, rain and load.
The constants are in `include/Route.hpp`.
