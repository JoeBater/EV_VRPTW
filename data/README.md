# data

Inputs for the solver. The `dataprep/` pipeline generates most of them.

Two **sample cities are committed: Bath (hilly) and Cambridge (flat)**. That's enough to
build and run the solver straight after cloning. Every other city's data is git-ignored
(the `*Bath*` / `*Cambridge*` allowlist is in the root `.gitignore`) and can be
regenerated with `dataprep/`.

```
data/
├── raw/          Original Solomon benchmark CSVs (committed)
├── processed/    City-mapped instances and short-path files (sample cities committed)
└── weather/      Hourly weather per city from Open-Meteo (sample cities committed)
```

## Datasets

| Dataset | Folder | Source | Committed |
|---|---|---|---|
| Solomon VRPTW benchmarks (c1, c2, r1, r2, rc1, rc2) | `raw/` | Solomon (1987) | all |
| City-mapped instances `<City>_<scale>_<benchmark>.csv` | `processed/instances/` | `dataprep/`: scaling, geocoding, elevation | Bath, Cambridge |
| Short paths | `processed/short_paths/` | `dataprep/` | Bath, Cambridge |
| Weather `<City>-weather.csv` | `weather/` | [Open-Meteo](https://open-meteo.com/) historical API | Bath, Cambridge |

To add another sample city, add a `!data/**/*<City>*` line to `.gitignore`.

### All cities

Aberystwyth, Bath, Belfast, Birmingham, Bristol, Cambridge, Cardiff, Carlisle, Chichester,
Derry, Glasgow, Inverness, Lancaster, Leeds, Liverpool, London, Newcastle, Plymouth,
Southampton, Wrexham.

## Regenerating

See [../dataprep/README.md](../dataprep/README.md).

<!-- TODO: add exact commands once the dataprep pipeline is in place -->
