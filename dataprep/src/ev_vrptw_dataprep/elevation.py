from functools import lru_cache

import numpy as np
import rasterio
from rasterio.transform import rowcol

# ============================================================
# ELEVATION
# ============================================================

@lru_cache(maxsize=2)
def _load_dem(dem_path):
    """
    Read a DEM band once and keep it in memory. Decompressing the full raster
    dominated run time when it was re-read for every edge.
    """
    with rasterio.open(dem_path) as src:
        return src.read(1), src.transform, src.nodata


def extract_elevations(dem_path, latlon_samples):
    """
    Extract elevations from DEM at given (lat, lon) points.
    Uses rowcol() + an in-memory band rather than src.sample()
    which was returning zeros for out-of-bounds points silently.
    """
    data, transform, nodata = _load_dem(str(dem_path))
    result = []
    for lat, lon in latlon_samples:
        try:
            row, col = rowcol(transform, lon, lat)
            # Guard against out-of-bounds indices
            if 0 <= row < data.shape[0] and 0 <= col < data.shape[1]:
                val = data[row, col]
                if nodata is not None and val == nodata:
                    result.append(np.nan)
                else:
                    result.append(float(val))
            else:
                result.append(np.nan)
        except Exception:
            result.append(np.nan)
    return result


def diagnose_dem(city_name, depot_lat, depot_lon, edge_samples, dem_path):
    """Confirm DEM covers the instance and returns sensible values."""
    with rasterio.open(dem_path) as src:
        print(f"\n  DEM diagnosis: {city_name}")
        print(f"  CRS:    {src.crs}")
        print(f"  Bounds: {src.bounds}")
        print(f"  NoData: {src.nodata}")
        print(f"  Res:    {src.res}")

        # Depot
        in_bounds = (src.bounds.left   <= depot_lon <= src.bounds.right and
                     src.bounds.bottom <= depot_lat <= src.bounds.top)
        print(f"  Depot ({depot_lat:.4f}, {depot_lon:.4f}) in bounds: {in_bounds}")
        if in_bounds:
            row, col = src.index(depot_lon, depot_lat)
            val = src.read(1)[row, col]
            print(f"  Depot elevation: {val:.1f}m")

        # First edge first 3 samples
        print(f"  First edge samples:")
        edge = edge_samples[0]
        elevs = extract_elevations(dem_path, edge["latlon_samples"][:3])
        for (lat, lon), elev in zip(edge["latlon_samples"][:3], elevs):
            print(f"    ({lat:.5f}, {lon:.5f}) → {elev}m")
