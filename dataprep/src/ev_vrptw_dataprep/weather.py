"""
Hourly historical weather per city from the Open-Meteo archive API.

Writes data/weather/open-meteo/<year>/<City>-weather-<year>.csv, one calendar
year of hourly rows in UTC, in the column order the solver's
Instance::readWeatherFile expects:

    month,day,hour,temperature_2m,dew_point_2m,rain,wind_speed_10m,
    wind_direction_10m,wind_gusts_10m,is_day

The solver reads month, day, hour, temperature_2m and rain by position.
"""
import argparse
import json
import os
import sys
import time
import urllib.error
import urllib.parse
import urllib.request

import pandas as pd

from .config import Paths
from .pipeline import load_cities

ARCHIVE_URL = "https://archive-api.open-meteo.com/v1/archive"

HOURLY_VARIABLES = [
    "temperature_2m",       # °C
    "dew_point_2m",         # °C
    "rain",                 # mm (preceding hour)
    "wind_speed_10m",       # km/h
    "wind_direction_10m",   # degrees
    "wind_gusts_10m",       # km/h
    "is_day",               # 0/1
]

COLUMNS = ["month", "day", "hour"] + HOURLY_VARIABLES

DEFAULT_YEARS = [2022, 2023, 2024, 2025]


def weather_path(paths, city_name, year):
    return paths.weather_dir / str(year) / f"{city_name}-weather-{year}.csv"


def fetch_hourly(lat, lon, year, retries=3):
    """Request one calendar year of hourly data (UTC) and return the JSON payload."""
    query = urllib.parse.urlencode({
        "latitude":   lat,
        "longitude":  lon,
        "start_date": f"{year}-01-01",
        "end_date":   f"{year}-12-31",
        "hourly":     ",".join(HOURLY_VARIABLES),
        "timezone":   "GMT",
    })
    url = f"{ARCHIVE_URL}?{query}"

    for attempt in range(1, retries + 1):
        try:
            with urllib.request.urlopen(url, timeout=60) as response:
                return json.load(response)
        except urllib.error.HTTPError as e:
            # 429 = rate limited; back off and retry
            if e.code != 429 or attempt == retries:
                raise
        except urllib.error.URLError:
            if attempt == retries:
                raise
        wait = 10 * attempt
        print(f"    request failed, retrying in {wait}s...")
        time.sleep(wait)


def to_frame(payload):
    """Convert an Open-Meteo hourly payload to the solver's weather CSV layout."""
    hourly = payload["hourly"]
    df     = pd.DataFrame({k: hourly[k] for k in HOURLY_VARIABLES})
    t      = pd.to_datetime(hourly["time"])

    df.insert(0, "month", t.month)
    df.insert(1, "day",   t.day)
    df.insert(2, "hour",  t.hour)
    df["is_day"] = df["is_day"].astype(int)
    return df[COLUMNS]


def download_city_weather(paths, city_name, lat, lon, year, force=False):
    output_path = weather_path(paths, city_name, year)
    if output_path.exists() and not force:
        print(f"  {city_name}: exists, skipping (use --force to replace): {output_path}")
        return output_path

    print(f"  {city_name}: fetching {year} ({lat:.4f}, {lon:.4f})...")
    payload = fetch_hourly(lat, lon, year)
    df      = to_frame(payload)

    missing = df[HOURLY_VARIABLES].isna().sum()
    if missing.any():
        print(f"    WARNING: missing values: {missing[missing > 0].to_dict()}")

    os.makedirs(output_path.parent, exist_ok=True)
    df.to_csv(output_path, index=False)
    print(f"    Written: {output_path} ({len(df)} rows, grid point "
          f"{payload['latitude']:.4f}, {payload['longitude']:.4f}, {payload['elevation']} m)")
    return output_path


def parse_args(argv=None):
    p = argparse.ArgumentParser(
        prog="python -m ev_vrptw_dataprep.weather",
        description="Download hourly historical weather per city from Open-Meteo.")
    p.add_argument("--cities", nargs="+",
                   help="cities to fetch (default: all in cities.csv)")
    p.add_argument("--years", nargs="+", type=int, default=DEFAULT_YEARS,
                   help=f"calendar years to fetch (default: {' '.join(map(str, DEFAULT_YEARS))})")
    p.add_argument("--force", action="store_true",
                   help="replace existing files")
    p.add_argument("--data-dir", help="data root (default: <repo>/data or $EV_VRPTW_DATA)")
    return p.parse_args(argv)


def main(argv=None):
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, "reconfigure"):
            stream.reconfigure(encoding="utf-8")

    args = parse_args(argv)
    if args.data_dir:
        os.environ["EV_VRPTW_DATA"] = args.data_dir
    paths  = Paths.default()
    cities = load_cities(paths.cities_file, args.cities)

    for _, city in cities.iterrows():
        for year in args.years:
            download_city_weather(paths, city["city"], city["latitude"], city["longitude"],
                                  year=year, force=args.force)


if __name__ == "__main__":
    main()
