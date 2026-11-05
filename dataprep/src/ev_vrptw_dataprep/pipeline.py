import argparse
import os
import sys
from pathlib import Path

import numpy as np
import pandas as pd

from .config import Paths, SUBSTEP_INTERVAL, metres_per_unit
from .dem import dem_path_for, ensure_dem
from .elevation import diagnose_dem, extract_elevations
from .geometry import (all_pairs_short_paths, compute_horizontal_distances,
                       compute_slopes, prim_mst, sample_edge, xy_to_latlon)
from .writers import (write_edge_profiles, write_edge_samples, write_scaled_instance,
                      write_short_paths)

# ============================================================
# INPUTS
# ============================================================

def load_solomon(instance_path):
    """Load a raw Solomon CSV and keep the standard columns only."""
    df = pd.read_csv(instance_path)
    df.columns = df.columns.str.strip().str.lower()

    df = df[['cust_no.', 'xcoord.', 'ycoord.', 'demand',
             'ready_time', 'due_date', 'service_time']].copy()
    df.columns = ['id', 'x', 'y', 'demand',
                  'ready_time', 'due_time', 'service_time']
    # Some source files store integers as floats (e.g. "1.0")
    return df.astype(int)


def load_cities(cities_file, names=None):
    cities = pd.read_csv(cities_file)
    if names:
        missing = set(names) - set(cities["city"])
        if missing:
            raise SystemExit(f"Unknown cities: {sorted(missing)}")
        cities = cities[cities["city"].isin(names)]
    return cities.reset_index(drop=True)


def list_benchmarks(instances_dir):
    return sorted(p.stem for p in Path(instances_dir).glob("*.csv"))

# ============================================================
# SHORT PATHS
# ============================================================

def generate_short_paths(benchmark, paths):
    """
    All-pairs MST paths for a benchmark.

    The MST depends only on the relative positions of the sites, so a uniform
    scale and a translation onto a city leave it unchanged: one file per
    benchmark serves every city and scale.
    """
    df          = load_solomon(paths.instances_dir / f"{benchmark}.csv")
    coordinates = df[['x', 'y']].values.astype(float)
    mst         = prim_mst(coordinates)

    os.makedirs(paths.short_paths_dir, exist_ok=True)
    output_path = paths.short_paths_dir / f"{benchmark}_short_paths.txt"
    write_short_paths(all_pairs_short_paths(mst, len(df)), output_path)
    return output_path

# ============================================================
# INSTANCES
# ============================================================

def generate_instance(
    city_name:     str,
    depot_lat:     float,
    depot_lon:     float,
    scale:         int,
    benchmark:     str,
    paths:         Paths,
    visualise:     bool = True,
    run_diagnosis: bool = False,
):
    print(f"\n{'='*60}")
    print(f"  {city_name} | {benchmark} | scale={scale}")
    print(f"{'='*60}")

    # --- paths ---
    stem              = f"{city_name}_{scale}_{benchmark}"
    instance_path     = paths.instances_dir / f"{benchmark}.csv"
    scaled_csv        = paths.scaled_instances_dir / f"{stem}.csv"
    edge_samples_file = paths.edge_samples_dir / f"{stem}_edge_samples.json"
    edge_profile_file = paths.edge_profiles_dir / f"{stem}.csv"
    png_path          = paths.plots_dir / f"{stem}_mst.png"
    dem_path          = dem_path_for(paths.dem_dir, city_name)

    for d in (paths.scaled_instances_dir, paths.edge_samples_dir, paths.edge_profiles_dir):
        os.makedirs(d, exist_ok=True)

    # --- 1. Load and normalise raw Solomon instance ---
    if not os.path.exists(instance_path):
        print(f"  SKIPPING: instance not found: {instance_path}")
        return

    df = load_solomon(instance_path)

    unit = metres_per_unit(scale)
    df["scaled_x"] = df["x"] * unit
    df["scaled_y"] = df["y"] * unit
    coordinates    = df[['scaled_x', 'scaled_y']].values
    n              = len(df)

    # Depot is always site 0
    depot_x = coordinates[0][0]
    depot_y = coordinates[0][1]
    print(f"  Loaded {n} sites | depot scaled: ({depot_x:.1f}, {depot_y:.1f})")

    # --- 2. MST ---
    mst = prim_mst(coordinates)
    print(f"  MST: {len(mst)} edges")

    # --- 3. Per-site lat/lon + substep sampling ---
    site_latlons = [
        xy_to_latlon(x, y, depot_lat, depot_lon, depot_x, depot_y)
        for x, y in coordinates
    ]
    df["lat"] = [ll[0] for ll in site_latlons]
    df["lon"] = [ll[1] for ll in site_latlons]

    edge_samples = []
    for u, v, _ in mst:
        A, B    = coordinates[u], coordinates[v]
        samples = sample_edge(A, B, SUBSTEP_INTERVAL)
        latlon_samples = [
            xy_to_latlon(x, y, depot_lat, depot_lon, depot_x, depot_y)
            for x, y in samples
        ]
        edge_samples.append({
            "start":          int(u),
            "end":            int(v),
            "samples":        [(float(x), float(y)) for x, y in samples],
            "latlon_samples": [(float(la), float(lo)) for la, lo in latlon_samples],
        })
    print(f"  Substep sampling: {len(edge_samples)} edges")

    # --- 4. Elevation ---
    if not os.path.exists(dem_path):
        print(f"  WARNING: DEM not found at {dem_path} — elevations skipped")
        df["elevation"] = np.nan
    else:
        if run_diagnosis:
            diagnose_dem(city_name, depot_lat, depot_lon, edge_samples, dem_path)

        # Per-site elevation
        site_elevations = extract_elevations(dem_path, site_latlons)
        df["elevation"] = site_elevations
        print(f"  Site elevation range: "
              f"{np.nanmin(site_elevations):.1f}m – "
              f"{np.nanmax(site_elevations):.1f}m")

        # Per-substep elevation, distances, slopes
        for edge in edge_samples:
            elevations         = extract_elevations(dem_path,
                                                    edge["latlon_samples"])
            edge["elevations"] = elevations
            edge["distances"]  = compute_horizontal_distances(edge["samples"])
            edge["slopes"]     = compute_slopes(elevations, edge["distances"])
        print(f"  Substep elevations extracted")

    # --- 5. Write outputs ---
    write_scaled_instance(df, scaled_csv)
    write_edge_samples(edge_samples, edge_samples_file)
    if "elevations" in edge_samples[0]:
        write_edge_profiles(edge_samples, edge_profile_file)

    # --- 6. Visualise ---
    if visualise:
        from .plots import visualise_instance
        os.makedirs(paths.plots_dir, exist_ok=True)
        visualise_instance(city_name, benchmark, scale,
                           coordinates, mst, edge_samples, png_path)

    print(f"  Done.")

# ============================================================
# RUN
# ============================================================

def parse_args(argv=None):
    p = argparse.ArgumentParser(
        prog="ev-vrptw-dataprep",
        description="Build city-mapped, terrain-aware instances from Solomon benchmarks.")
    p.add_argument("--cities", nargs="+",
                   help="cities to generate (default: all in cities.csv)")
    p.add_argument("--benchmarks", nargs="+",
                   help="benchmarks to generate (default: all in data/raw/solomon)")
    p.add_argument("--scales", nargs="+", type=int, default=[5],
                   help="grid size in km (default: 5)")
    p.add_argument("--plots", choices=["none", "first", "all"], default="first",
                   help="which instances to plot (default: first)")
    p.add_argument("--diagnose", action="store_true",
                   help="print DEM diagnostics for the first instance")
    p.add_argument("--data-dir", type=Path,
                   help="data root (default: <repo>/data or $EV_VRPTW_DATA)")
    return p.parse_args(argv)


def main(argv=None):
    # Windows consoles default to cp1252, which can't print the log's arrows/dashes
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, "reconfigure"):
            stream.reconfigure(encoding="utf-8")

    args  = parse_args(argv)
    paths = Paths.default()
    if args.data_dir:
        os.environ["EV_VRPTW_DATA"] = str(args.data_dir)
        paths = Paths.default()

    cities     = load_cities(paths.cities_file, args.cities)
    benchmarks = args.benchmarks or list_benchmarks(paths.instances_dir)

    # Step 1: ensure all DEMs present and covering the right area
    print("="*60)
    print("STEP 1: DEM acquisition")
    print("="*60)
    for _, city in cities.iterrows():
        ensure_dem(paths.dem_dir, city["city"], city["latitude"], city["longitude"])

    # Step 2: short paths, one per benchmark
    print("\n" + "="*60)
    print("STEP 2: Short paths")
    print("="*60)
    for benchmark in benchmarks:
        generate_short_paths(benchmark, paths)

    # Step 3: generate all instances
    # First instance gets diagnosis + visualisation as a sanity check
    print("\n" + "="*60)
    print("STEP 3: Instance generation")
    print("="*60)
    first = True
    for _, city in cities.iterrows():
        for scale in args.scales:
            for benchmark in benchmarks:
                generate_instance(
                    city_name     = city["city"],
                    depot_lat     = city["latitude"],
                    depot_lon     = city["longitude"],
                    scale         = scale,
                    benchmark     = benchmark,
                    paths         = paths,
                    visualise     = args.plots == "all" or (args.plots == "first" and first),
                    run_diagnosis = args.diagnose and first,
                )
                first = False


if __name__ == "__main__":
    main()
