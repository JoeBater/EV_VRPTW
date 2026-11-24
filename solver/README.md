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
| `MST` | Paths between pairs of customers along the minimum spanning tree: read per pair from a `*_short_paths.txt` file, or walked on the tree read from a `*_mst.txt` edge list. Legs are expanded into `SubStep`s. |
| `LNS` | Adaptive Large Neighbourhood Search (see [Search](#search)). |
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
benchmark is solved with Solomon's capacity (200, or 1000 for r2/rc2), offering its best-known
vehicle count. The fleet is an upper bound: the LNS can leave vehicles at the depot, and the
summary's `vehicles` column counts the routes used. Travel here is far faster than Solomon's one
unit per minute, so the windows are loose and fewer vehicles often suffice (r101: 8, not 19). If the
flatland solution has late stops or overload, exp0 offers another vehicle and solves again (up to 5
extra), so every baseline is clean. Matching Solomon's optimum is not the aim.

exp0 and exp0a warm-start from the published solutions in `results/solomon_solutions/<benchmark>.txt`
(SINTEF format, `Route 1 : 5 3 7 ...`; SINTEF customer k is solver index k, depot 0). The
first attempt uses the published routes and fleet; the LNS returns the best solution it has seen,
so a baseline is never worse than its start. Extra-vehicle attempts start from a random assignment.
The summary's `start` column says which (`solomon` or `random`). Every published solution is
feasible in this model on the flat complete graph, at exactly Solomon's distance × 50 m,
because travel here is never slower than Solomon's one unit per minute.

The extended benchmarks (`<benchmark>_200`, `_400`, from dataprep `extend.py`; each size contains
the one before) have no published solutions. They warm-start from the previous size's baseline
(`r101` for `r101_200`, `r101_200` for `r101_400`):
- its routes are kept as they are;
- the stores added since are split by angle around the depot into extra vehicles;
- the fleet starts at the previous fleet scaled by size, and each attempt adds ~10% more, up to
  5 more attempts.

The start file is `extended_start.txt` and the `start` column names the previous benchmark. So run
the sizes in order: 100, then 200, then 400.

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
./build/release/solver/ev_vrptw                  # exp0: solve all flatland baselines (about 1 min each at 100 stores)
./build/release/solver/ev_vrptw --bench r105     # just one (repeatable); --iterations N to change the LNS budget
./build/release/solver/ev_vrptw --exp 0a         # the same on the complete graph
./build/release/solver/ev_vrptw --bench c107 --random-start   # ignore published/smaller solutions (tests the search)
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
| Instance CSV `data/processed/instances/<City>/<City>_<scale>_<benchmark>.csv` | `id,x,y,demand,ready_time,due_time,service_time,scaled_x,scaled_y,lat,lon,elevation` (first row is the depot; read by column position) |
| Short paths `data/processed/short_paths/<benchmark>_short_paths.txt` | One line per pair, `from to: <steps>`; lines starting with `#` are ignored |
| MST `data/processed/mst/<benchmark>_mst.txt` | Used when there is no short-paths file. One tree edge per line, `u v` (0-based, depot 0); the path between two sites is found on the tree. Same paths as the short-paths format, which grows with the square of the sites (~400 MB at 1000) |
| Weather CSV `data/weather/open-meteo/<year>/<City>-weather-<year>.csv` | Open-Meteo hourly data: `month,day,hour,temperature_2m,dew_point_2m,rain,...` |

## TODO

- [x] Replace hard-coded `C:/Users/...` paths with a data root and an output root
- [ ] Choose the experiment and its parameters from the command line or a config file, not constructors
- [ ] Tidy: remove `using namespace std` from headers and `<bits/stdc++.h>` from `Route.hpp`
- [ ] Add tests (e.g. a small instance with a known route cost)
- [x] **Strengthen the LNS before exp2** (see [Search](#search)).
- [x] **Loading rule was off by one stop.** The leg into stop *i* (counting from 0) carried the total
  less the demands of stops 1..*i*, not 0..*i*-1: the next stop's demand had been dropped instead of the
  last one's. Fixed on 2026-11-23; it changes speed and energy slightly, so results from before differ.

## Search

`LNS` is an adaptive large neighbourhood search (after Ropke & Pisinger, 2006). Each iteration:

1. **Destroy**: removes *q* stores, *q* drawn from [min(0.1n, 30), min(0.4n, 60)] for *n* stores, by one of
   - random;
   - worst: the biggest detours (and late stops);
   - related (Shaw): close in space, service time and demand;
   - a whole route, the shortest of three, so a vehicle can be freed;
   - late stops and stores related to them.
2. **Repair**: reinserts them by greedy, regret-2 or regret-3 insertion, each with or without noise.
   Positions are costed with an *estimate*: travel minutes along the network with no rain, slowed by
   the load as in `Route`, and the slack each route has before its windows. It respects capacity and
   time windows through the cost.
3. **Evaluate**: only the routes that changed are re-simulated, in full, without the per-minute log.
   That exact cost (distance, late stops, minutes late, overload) decides.
4. **Accept** by simulated annealing (a 5% worse solution starts at a 50% chance, cooling to 0.2% of
   that); the best solution seen is kept.

Operators are chosen by weights that adapt every 100 iterations to how often each finds new best,
better or accepted solutions. At the end the best solution is polished route by route (2-opt and
moving 1-3 consecutive stops), keeping only moves the full simulation confirms. The random seed is
fixed, so a run is reproducible.

Against the previous search (pure descent with schedule-blind repairs), from a random start, 20,000 iterations:

| Benchmark | Previous | Now | Previous exp0 baseline (published start, 250,000 iterations) |
|---|---|---|---|
| c107 | 355.7 km, 11 vehicles, 60 s | 76.8 km, 10 vehicles, 4 s | 77.1 km, 10 vehicles |
| r101 | 127.8 km, 19, 27 s | 83.7 km, 8, 4 s | 122.0 km, 19 |
| rc105 | 253.3 km, 13, 32 s | 104.3 km, 11, 3 s | 150.8 km, 13 |
| r205 | 177.3 km, 3, 28 s | 91.6 km, 3, 7 s | 112.6 km, 3 |

On the flat complete graph, c107 reaches 41.47 km in 3,000 iterations from a random start; Solomon's
optimum is 41.45 km (828.94 units).

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
