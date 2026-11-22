"""
Extend a Solomon layout with extra stores, growing its road network instead of rebuilding it.

Each new store:
  1. is drawn at a random whole-number grid point inside the base layout's bounding box;
  2. is kept only if, placed in every city, it and the new road to it are on land
     (no bridges or fords: every ~100 m point of the road must be above WATER_M);
  3. joins the network by one new road to the nearest existing node (original or new).

The base network is never changed, so the result is the base MST plus one spur per store: a tree,
but not the MST of all the points. It is saved as an edge list (processed/mst/<name>_mst.txt) and the
pipeline uses it as given rather than running Prim again.

Demand and time window come from a random "donor" customer of the same benchmark, so each
benchmark keeps its own character (r101 tight, r104 mostly unconstrained):
  - demand: the donor's;
  - a donor whose window spans the whole horizon (ready 0) gives (0, latest feasible due);
  - otherwise the donor's window width, placed uniformly where the store is reachable from the
    depot and the vehicle can still return before the depot closes (Solomon time: 1 unit = 1 min).
Service time is SERVICE (10, as for every R customer).

Usage, from dataprep/:
  PYTHONPATH=src python -m ev_vrptw_dataprep.extend --base r101 \
      --benchmarks r101 r104 r105 r204 r205 r208 --add 100 --seed 1
writes raw/extended/<benchmark>_<customers>.csv and processed/mst/<benchmark>_<customers>_mst.txt;
then run the pipeline on the new names (e.g. --benchmarks r101_200) for the city files.
"""
import argparse
import math
import os
import sys

import numpy as np
import pandas as pd

from .config import Paths, SUBSTEP_INTERVAL, metres_per_unit
from .dem import dem_path_for
import rasterio
from rasterio.transform import rowcol
from rasterio.windows import from_bounds
from .geometry import prim_mst, sample_edge, xy_to_latlon
from .pipeline import load_cities, load_solomon, raw_instance_path, read_tree

WATER_M  = 1.5    # below this the DEM is water (as for the city points in cities.csv)
SERVICE  = 10
RAW_COLS = ["CUST_NO.", "XCOORD.", "YCOORD.", "DEMAND", "READY_TIME", "DUE_DATE", "SERVICE_TIME"]


def base_network(benchmark, paths):
    """Sites and tree edges of a benchmark: its saved tree if it has one, else Prim's MST."""
    df    = load_solomon(raw_instance_path(benchmark, paths))
    edges = read_tree(benchmark, paths)
    if edges is None:
        edges = [(u, v) for u, v, _ in prim_mst(df[["x", "y"]].values.astype(float))]
    return df, edges


def city_dems(cities, paths, margin_deg=0.1):
    """Each city's DEM cropped to its surroundings, read once: (city row, band, transform, nodata)."""
    dems = []
    for city in cities.itertuples(index=False):
        with rasterio.open(dem_path_for(paths.dem_dir, city.city)) as src:
            window = from_bounds(city.longitude - 2 * margin_deg, city.latitude - margin_deg,
                                 city.longitude + 2 * margin_deg, city.latitude + margin_deg, src.transform)
            window = window.round_offsets().round_lengths()
            dems.append((city, src.read(1, window=window), src.window_transform(window), src.nodata))
    return dems


def on_land(points_xy, depot_xy, dems, unit):
    """True if every grid point (x, y) is above WATER_M in every city (same pixel rule as extract_elevations)."""
    xy = np.asarray(points_xy, dtype=float) * unit
    dx, dy = depot_xy[0] * unit, depot_xy[1] * unit
    for city, band, transform, nodata in dems:
        lat, lon = xy_to_latlon(xy[:, 0], xy[:, 1], city.latitude, city.longitude, dx, dy)
        rows, cols = rowcol(transform, np.atleast_1d(lon), np.atleast_1d(lat))
        rows, cols = np.asarray(rows), np.asarray(cols)
        if ((rows < 0) | (rows >= band.shape[0]) | (cols < 0) | (cols >= band.shape[1])).any():
            return False
        z = band[rows, cols].astype(float)
        if (nodata is not None and (z == nodata).any()) or (z < WATER_M).any():
            return False
    return True


def grow_layout(base, base_edges, add, dems, scale, rng, max_draws=100_000):
    """Add `add` stores; returns their (x, y), the new edges and the number of draws rejected."""
    unit     = metres_per_unit(scale)
    coords   = base[["x", "y"]].to_numpy(dtype=float)
    depot    = coords[0]
    lo, hi   = coords[1:].min(axis=0).astype(int), coords[1:].max(axis=0).astype(int)
    new_xy, new_edges, rejected = [], [], 0
    while len(new_xy) < add:
        if rejected > max_draws:
            raise RuntimeError(f"gave up after {rejected} draws on water")
        p = np.array([rng.integers(lo[0], hi[0] + 1), rng.integers(lo[1], hi[1] + 1)], dtype=float)
        nearest = int(np.argmin(np.hypot(*(coords - p).T)))      # lowest index on ties
        road = [np.array(s) / unit for s in sample_edge(coords[nearest] * unit, p * unit, SUBSTEP_INTERVAL)]
        if not on_land(road, depot, dems, unit):                  # road includes the store itself
            rejected += 1
            continue
        new_edges.append((nearest, len(coords)))
        coords = np.vstack([coords, p])
        new_xy.append(p)
    return np.array(new_xy, dtype=int), new_edges, rejected


def new_customers(benchmark_df, xy, rng):
    """Demand and time window for each new store, from random donors of the same benchmark."""
    depot  = benchmark_df.iloc[0]
    donors = benchmark_df.iloc[1:]
    horizon = int(depot.due_time)
    rows = []
    for x, y in xy:
        donor = donors.iloc[rng.integers(len(donors))]
        t   = math.hypot(x - depot.x, y - depot.y)
        lo  = math.ceil(t)                                   # earliest arrival
        hi  = math.floor(horizon - t - SERVICE)              # latest service start that still gets home
        if donor.ready_time == 0:                            # whole-horizon window
            ready, due = 0, hi
        else:
            width = min(int(donor.due_time - donor.ready_time), hi - lo)
            ready = int(rng.integers(lo, hi - width + 1))
            due   = ready + width
        assert ready + SERVICE <= due, (x, y, ready, due)
        rows.append(dict(x=int(x), y=int(y), demand=int(donor.demand),
                         ready_time=ready, due_time=due, service_time=SERVICE))
    return pd.DataFrame(rows)


def write_raw(df, path):
    out = df[["id", "x", "y", "demand", "ready_time", "due_time", "service_time"]].copy()
    out.columns = RAW_COLS
    out.to_csv(path, index=False)
    print(f"  Written: {path}")


def write_tree(edges, path):
    with open(path, "w") as f:
        f.write("# MST edges: u v (0-based sites, depot = 0)\n")
        f.writelines(f"{u} {v}\n" for u, v in edges)
    print(f"  Written: {path}")


def extend(base, benchmarks, add, seed, paths, scale=5, city_names=None):
    rng    = np.random.default_rng(seed)
    cities = load_cities(paths.cities_file, city_names)
    base_df, base_edges = base_network(base, paths)
    for b in benchmarks:                                    # they must share the base's layout
        other = load_solomon(raw_instance_path(b, paths))
        if not np.array_equal(other[["x", "y"]].values, base_df[["x", "y"]].values):
            raise SystemExit(f"{b} does not share {base}'s layout")

    xy, new_edges, rejected = grow_layout(base_df, base_edges, add, city_dems(cities, paths), scale, rng)
    edges = base_edges + new_edges
    total = len(base_df) - 1 + add
    print(f"{base}: added {add} stores ({rejected} draws rejected for water) -> {total} customers")

    os.makedirs(paths.extended_dir, exist_ok=True)
    os.makedirs(paths.mst_dir, exist_ok=True)
    for b in benchmarks:
        df  = load_solomon(raw_instance_path(b, paths))
        new = new_customers(df, xy, rng)
        new.insert(0, "id", range(len(df) + 1, len(df) + 1 + len(new)))
        name = f"{b.split('_')[0]}_{total}"
        write_raw(pd.concat([df, new], ignore_index=True), paths.extended_dir / f"{name}.csv")
        write_tree(edges, paths.mst_dir / f"{name}_mst.txt")


def main(argv=None):
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, "reconfigure"):
            stream.reconfigure(encoding="utf-8")
    p = argparse.ArgumentParser(prog="ev-vrptw-extend", description=__doc__.split("\n\n")[0])
    p.add_argument("--base", required=True, help="benchmark whose layout and network to extend")
    p.add_argument("--benchmarks", nargs="+", required=True,
                   help="benchmarks sharing that layout; each gets its own demands and windows")
    p.add_argument("--add", type=int, default=100, help="stores to add (default 100)")
    p.add_argument("--seed", type=int, default=1)
    p.add_argument("--cities", nargs="+", help="cities that must be dry (default: all)")
    args = p.parse_args(argv)
    extend(args.base, args.benchmarks, args.add, args.seed, Paths.default(), city_names=args.cities)


if __name__ == "__main__":
    main()
