# data

Inputs for the solver. The `dataprep/` pipeline generates most of them, so only small
reference files are committed here.

```
data/
├── raw/          Original Solomon benchmark CSVs (small; committed)
├── processed/    City-mapped instances and short-path files (generated; git-ignored)
└── weather/      Hourly weather per city from Open-Meteo (downloaded; git-ignored)
```

## Datasets

| Dataset | Folder | Source | Committed? |
|---|---|---|---|
| Solomon VRPTW benchmarks (c1, c2, r1, r2, rc1, rc2) | `raw/` | Solomon (1987) | yes |
| City-mapped instances `<City>_<scale>_<benchmark>.csv` | `processed/instances/` | `dataprep/`: scaling, geocoding, elevation | no (~27 MB) |
| Short paths `<benchmark>_short_paths.txt` | `processed/short_paths/` | `dataprep/` | no |
| Weather `<City>-weather.csv` | `weather/` | [Open-Meteo](https://open-meteo.com/) historical API | no (~43 MB) |

### Cities

Aberystwyth, Bath, Belfast, Birmingham, Bristol, Cambridge, Cardiff, Carlisle, Chichester,
Derry, Glasgow, Inverness, Lancaster, Leeds, Liverpool, London, Newcastle, Plymouth,
Southampton, Wrexham.

## Regenerating

See [../dataprep/README.md](../dataprep/README.md).

<!-- TODO: add exact commands once the dataprep pipeline is in place -->
