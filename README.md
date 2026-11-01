# EV_VRPTW

An **Electric-Vehicle Vehicle Routing Problem with Time Windows** solver, built on the
Solomon VRPTW benchmarks but placed in real UK cities so that terrain and weather affect
the vehicles' energy use.

Each benchmark instance is mapped onto a city's geography (latitude, longitude, elevation),
and routes are simulated against hourly weather data. Battery state-of-charge along each
route depends on:

- **Terrain**: road slope from elevation differences, with regenerative braking on descents
- **Load**: payload mass changes as deliveries are made
- **Speed and acceleration**: the speed profile for each leg length, plus acceleration impulses
- **Weather**: HVAC draw that depends on temperature; rain affects speed and service times

Solutions are found with **Large Neighbourhood Search** (destroy/repair plus 2-opt), with
penalties for late arrival and exceeding capacity.

## Repository layout

```
EV_VRPTW/
├── solver/         C++ solver: problem model, energy model, LNS, experiment drivers
├── dataprep/       Python pipeline that builds the city-mapped instances and weather inputs
├── data/           Input data (mostly not committed; see data/README.md)
├── results/        Run outputs (git-ignored)
├── docs/           Write-ups and figures
├── CMakeLists.txt
└── CMakePresets.json
```

Each folder has its own README with details.

## Quick start (solver)

Requirements: CMake ≥ 3.20, a C++17 compiler (tested with MinGW-w64 g++ 14), and Ninja.

```bash
cmake --preset release
cmake --build --preset release
./build/release/solver/ev_vrptw
```

There is also a `debug` preset. See [solver/README.md](solver/README.md).

The Bath and Cambridge sample data is committed, so this runs straight after cloning.
Outputs go to `results/`.

## Data

Input data comes from the Solomon benchmarks, geocoding and elevation lookups, and
Open-Meteo hourly weather. See [data/README.md](data/README.md) for the expected layout
and [dataprep/README.md](dataprep/README.md) for how the inputs are generated.

## License

MIT. See [LICENSE](LICENSE).
