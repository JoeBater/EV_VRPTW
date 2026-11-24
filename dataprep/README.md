# dataprep

Python pipeline that builds the solver's inputs: Solomon VRPTW benchmarks placed in real
UK cities, with terrain from the Copernicus 30 m digital elevation model.

## What it does

For each **city × scale × benchmark**:

1. **DEM**: downloads the Copernicus GLO-30 tiles that cover the city from the public AWS
   bucket (`copernicus-dem-30m`, no credentials needed) and merges them into
   `data/dem/<City>/<City>_dem.tif`. Tiles are cached, so this only happens once per city.
2. **Scale**: maps Solomon's 100 × 100 grid onto a `scale` km square (scale 5 → 1 unit = 50 m).
3. **Geolocate**: places the **depot on the city's point in `data/raw/cities.csv`** (the
   centre, or nearby for coastal and river cities; see [Known DEM issues](#known-dem-issues)) and
   converts every site to latitude and longitude.
4. **MST**: builds a Prim minimum spanning tree over the sites. Vehicles travel along it.
5. **Elevation**: looks up the elevation of every site, and of points every 100 m along each
   MST edge, giving the terrain profile the solver drives over.

Plus, once per **benchmark**:

6. **Short paths**: the intermediate MST nodes for every pair of sites. A uniform scale and a
   move onto a city don't change the MST, so one file serves every city and scale (this is
   checked in `tests/`).

## Outputs

| Output | Path | Read by | Committed |
|---|---|---|---|
| Instance | `data/processed/instances/<City>/<City>_<scale>_<benchmark>.csv` | solver | sample cities |
| Short paths | `data/processed/short_paths/<benchmark>_short_paths.txt` | solver (`MST`) | yes |
| Edge profiles | `data/processed/edge_profiles/<City>_<scale>_<benchmark>.csv` | solver (`MST`) | sample cities |
| Edge samples | `data/processed/edge_samples/<City>_<scale>_<benchmark>_edge_samples.json` | analysis | no |
| MST plot | `data/processed/plots/<City>_<scale>_<benchmark>_mst.png` | you | no |
| DEM cache | `data/dem/<City>/` | pipeline | no |

Instance columns: `id,x,y,demand,ready_time,due_time,service_time,scaled_x,scaled_y,lat,lon,elevation`.
Short-path lines: `from to: step step …` (0-based site indices, with the endpoints excluded).
Edge-profile rows: `start,end,length,dz`, one per ~100 m segment of each MST edge, in order
from `start` to `end` (the solver walks them in reverse, with `dz` negated, for `end` → `start`).

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

## Extended instances

`extend.py` grows a Solomon layout with extra stores while keeping its road network:

```bash
PYTHONPATH=src python -m ev_vrptw_dataprep.extend --base r101     --benchmarks r101 r104 r105 r204 r205 r208 --add 100 --seed 1
PYTHONPATH=src python -m ev_vrptw_dataprep --benchmarks r101_200 r104_200 r105_200 r204_200 r205_200 r208_200
```

```bash
PYTHONPATH=src python -m ev_vrptw_dataprep.extend --base c107 --benchmarks c107 c108 c109 --placement near
PYTHONPATH=src python -m ev_vrptw_dataprep.extend --base rc101 \
    --benchmarks rc101 rc104 rc105 rc107 rc201 rc204 rc207 --placement mixed
```

How each new store is placed:
- It is drawn by the `--placement` rule, then rounded to a whole-number grid point:
  - `random` (R): anywhere in the base layout's bounding box;
  - `near` (C): 50–100 m (up to ~112 m after rounding) in a random direction from a random existing
    customer, original or added, so the clusters grow;
  - `mixed` (RC): a coin flip between the two for each store.
- It is kept only if it **and its new road** are on land (above 1.5 m) in **every** city. Roads may not
  cross water: no bridges or fords.
- It joins the nearest existing node (original or new) by one road.

The result is the base MST plus one spur per store. It is a tree, but not the MST of all the sites, so it
is saved as an edge list in `data/processed/mst/<name>_mst.txt`. The pipeline uses that list instead of
running Prim again, and the solver walks paths on it.

Later rounds extend an extended layout, keeping its network, so each size contains the one before:

```bash
PYTHONPATH=src python -m ev_vrptw_dataprep.extend --base r101_200 \
    --benchmarks r101_200 r104_200 r105_200 r204_200 r205_200 r208_200 --add 200    # -> r101_400 ...
```

A later round is seeded with the seed and the base size: the plain seed would replay the first round's
draws and put new stores on the same points.

All benchmarks sharing a layout get the same new stores. Each gets its own demands and windows, copied
from a random donor customer of that benchmark:
- the donor's demand;
- a whole-horizon window if the donor has one, else the donor's window width, placed where the store is
  reachable and the vehicle can still return before the depot closes;
- the donor's service time (10 for R and RC, 90 for C).

New stores are numbered after the originals (sites 101+), so the first 101 rows and the original edge
profiles are unchanged. Stores may share a site (co-located customers).

Outputs: the raw instances in `data/raw/extended/<benchmark>_<customers>.csv` (for example `r101_200`) and
the trees in `data/processed/mst/`. The pipeline then writes the city files
`instances/<City>/<City>_5_r101_200.csv` and `edge_profiles/<City>_5_r101_200.csv`.

## Weather

```bash
PYTHONPATH=src python -m ev_vrptw_dataprep.weather --cities Bath Cambridge
```

Downloads hourly data per city and calendar year (default **2022–2025**) from the
[Open-Meteo archive API](https://open-meteo.com/en/docs/historical-weather-api) to
`data/weather/open-meteo/<year>/<City>-weather-<year>.csv`. It's free and needs no key.
Existing files are kept unless you pass `--force`. Other options: `--years` and `--data-dir`.

Columns, all in UTC: `month,day,hour,temperature_2m,dew_point_2m,rain,wind_speed_10m,wind_direction_10m,wind_gusts_10m,is_day`.
The solver reads month, day, hour, temperature and rain by position.

> **Note:** these files replace the originals, which had two different layouts. Compared
> with the original 2024 file, rain and wind are identical, but temperature and dew point are
> consistently about 0.6 °C lower. That reflects today's API; no request option tried
> reproduced the old values. Results from before the switch may differ slightly.

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
│   ├── extend.py       add stores to a layout, growing its network
│   └── weather.py      Open-Meteo download, CLI
├── notebooks/
│   ├── copernicus-elevation-model.ipynb   original notebook this package came from
│   ├── time-window-feasibility.ipynb      can every customer be served inside its window?
│   └── mst-elevation-profile.ipynb        the MST coloured by elevation; sagittal view between two sites
└── tests/
```

## Known DEM issues

Copernicus GLO-30 is a radar surface model, not bare earth. Not fixed yet:

- **Artefact pits.** Radar DSMs can read several metres too low between tall buildings.
  Cardiff r105 site 96 (51.4788, −3.1753) sits in a ~200 × 70 m pit at −7 m where the ground
  is ~10 m, so trips through it pay a false ~17 m climb. A despike (replace pixels well below
  their neighbourhood median) would fill these; a wider median would also cut roof and tree
  noise, which is most of the slope signal in flat cities such as Cambridge.
- **Buildings read as terrain.** Worst in central London: its r105 mean grade is 4.0%
  (same as Bristol or Glasgow) where the ground is mostly 1–2%, and the highest sites
  (Soho, St James's, Bloomsbury at 40–46 m) are on rooftops ~20 m above the street.
- **Water is flattened to ~0–1 m**, so a site on a river, dock or the sea gets a false drop
  and climb from its banks. To avoid this, **the city points in `cities.csv` are not all city
  centres**: each coastal or river city is moved by the smallest shift (250 m steps) that puts
  every site and every 100 m road sample of every scale-5 benchmark above 1.5 m. Liverpool
  moved ~6.5 km to Alder Hey Children's Hospital (it had 39 of 101 r105 sites in the Mersey);
  Derry, Plymouth, Southampton, London, Belfast, Glasgow, Inverness, Newcastle, Cardiff,
  Aberystwyth and Lancaster moved 0.5–2.6 km (before: up to 18% of sites on water, in Derry).
  The other cities had nothing on water. A new city or benchmark layout needs the same check.

## Backlog

- [ ] **Switch to a bare-earth DEM: [FABDEM](https://doi.org/10.5523/bris.s5hqmjcdj8yo2ibzi9b4ew3sn)**
  (Copernicus GLO-30 with buildings and trees removed; same 30 m grid and 1° tiles; free
  for research, CC BY-NC-SA 4.0). It would fix the rooftop and tree noise and probably the
  artefact pits above, making a separate despike unnecessary. A major change: every city's
  instances and edge profiles, including the committed Bath and Cambridge ones, would be
  regenerated, and results compared before and after.

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

