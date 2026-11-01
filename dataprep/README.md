# dataprep

Python pipeline that builds the solver's inputs: Solomon VRPTW benchmarks placed in real
UK cities, with terrain from the Copernicus 30 m digital elevation model.

## What it does

For each **city × scale × benchmark**:

1. **DEM**: downloads the Copernicus GLO-30 tiles that cover the city from the public AWS
   bucket (`copernicus-dem-30m`, no credentials needed) and merges them into
   `data/dem/<City>/<City>_dem.tif`. Tiles are cached, so this only happens once per city.
2. **Scale**: maps Solomon's 100 × 100 grid onto a `scale` km square (scale 5 → 1 unit = 50 m).
3. **Geolocate**: places the **depot on the city centre** (from `data/raw/cities.csv`) and
   converts every site to latitude and longitude.
4. **MST**: builds a Prim minimum spanning tree over the sites. Vehicles travel along it.
5. **Elevation**: looks up the elevation of every site, and of points every 100 m along each
   MST edge, to give per-substep slopes.

Plus, once per **benchmark**:

6. **Short paths**: the intermediate MST nodes for every pair of sites. A uniform scale and a
   move onto a city don't change the MST, so one file serves every city and scale (this is
   checked in `tests/`).

## Outputs

| Output | Path | Read by | Committed |
|---|---|---|---|
| Instance | `data/processed/instances/<City>_<scale>_<benchmark>.csv` | solver | sample cities |
| Short paths | `data/processed/short_paths/<benchmark>_short_paths.txt` | solver (`MST`) | yes |
| Edge samples | `data/processed/edge_samples/<City>_<scale>_<benchmark>_edge_samples.json` | analysis | no |
| MST plot | `data/processed/plots/<City>_<scale>_<benchmark>_mst.png` | you | no |
| DEM cache | `data/dem/<City>/` | pipeline | no |

Instance columns: `id,x,y,demand,ready_time,due_time,service_time,scaled_x,scaled_y,lat,lon,elevation`.
Short-path lines: `from to: step step …` (0-based site indices, with the endpoints excluded).

## Usage

From `dataprep/`, with numpy, pandas, rasterio, boto3 and matplotlib installed:

```bash
# Everything: all cities in cities.csv, all benchmarks in data/raw/solomon, 5 km grid
PYTHONPATH=src python -m ev_vrptw_dataprep

# Just the committed sample cities
PYTHONPATH=src python -m ev_vrptw_dataprep --cities Bath Cambridge

# Selected benchmarks, both grid sizes, plot every instance
PYTHONPATH=src python -m ev_vrptw_dataprep --benchmarks r101 c107 --scales 5 10 --plots all
```

Or install it, which also adds an `ev-vrptw-dataprep` command:

```bash
pip install -e ".[dev]"
ev-vrptw-dataprep --cities Bath Cambridge
```

Options: `--cities`, `--benchmarks`, `--scales` (km, default 5), `--plots none|first|all`
(default `first`), `--diagnose` (DEM sanity check on the first instance), and `--data-dir`
(or the `$EV_VRPTW_DATA` environment variable) to use a data root outside the repo.

Two sample cities with all 16 benchmarks take about 10 seconds once the DEMs are cached.

## Weather

```bash
PYTHONPATH=src python -m ev_vrptw_dataprep.weather --cities Bath Cambridge
```

Downloads one calendar year (default **2024**, the year of the original files) of hourly
data per city from the [Open-Meteo archive API](https://open-meteo.com/en/docs/historical-weather-api)
to `data/weather/open-meteo/<City>-weather.csv`. It's free and needs no key. Existing files are
kept unless you pass `--force`. Other options: `--year` and `--data-dir`.

Columns, all in UTC: `month,day,hour,temperature_2m,dew_point_2m,rain,wind_speed_10m,wind_direction_10m,wind_gusts_10m,is_day`.
The solver reads month, day, hour, temperature and rain by position.

> **Known difference from the original files.** Today's API gives the same rain, wind and
> grid cell as the committed Bath and Cambridge files, but temperature and dew point are
> consistently about 0.6 °C lower. No request option tried (elevation, model) reproduces
> the old values exactly. The committed files have not been regenerated.

## Tests

```bash
python -m pytest
```

## Layout

```
dataprep/
├── pyproject.toml
├── src/ev_vrptw_dataprep/
│   ├── config.py       paths (relative to the repo) and constants
│   ├── dem.py          Copernicus tile lookup, download, merge
│   ├── geometry.py     Prim MST, MST routes, edge sampling, xy→lat/lon, slopes
│   ├── elevation.py    DEM sampling and diagnostics
│   ├── writers.py      CSV / short-path / JSON writers
│   ├── plots.py        MST and elevation plot
│   ├── pipeline.py     generate_instance, generate_short_paths, CLI
│   └── weather.py      Open-Meteo download, CLI
├── notebooks/
│   └── copernicus-elevation-model.ipynb   original notebook this package came from
└── tests/
```

## Changes from the original notebook

- Paths are relative to the repo (or `--data-dir`) instead of hard-coded under `C:/Users/...`.
- **`scale` is now applied.** The notebook always used 50 m per unit, so its `_10` files were
  copies of the `_5` files.
- Short paths are written once per benchmark instead of once per city and scale.
- The DEM is read once per city instead of once per MST edge, which takes runs from about
  an hour to seconds.
- Integer columns are written as integers (some raw Solomon files store them as `1.0`).

Otherwise the outputs were checked against the notebook's: all 30 Bath and Cambridge
scale-5 instances match to within 1e-9, and the short paths are byte-identical.

## Not yet migrated

- `Last-mile-robots/clean_weather_files.ipynb` (EnergyPlus EPW + synthetic rain → `data/weather/cleaned/`).
  The solver doesn't use these files.
