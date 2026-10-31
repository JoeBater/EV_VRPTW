import os
import shutil

import numpy as np
import rasterio
from rasterio.merge import merge

# ============================================================
# TILE INDEX HELPERS
# ============================================================

def lat_tile_index(lat):
    """
    SW corner latitude index for a 1°x1° Copernicus tile.
    UK latitudes are all positive so floor works correctly.
    """
    return int(np.floor(lat))

def lon_tile_index(lon):
    """
    SW corner longitude index for a 1°x1° Copernicus tile.
    For negative longitudes (W), floor gives the correct SW corner:
      lon=-1.89 → floor=-2 → W002 tile covers -2.0 to -1.0  ✓
      lon=-1.00 → floor=-1 → W001 tile covers -1.0 to  0.0  ✓
      lon= 0.12 → floor= 0 → E000 tile covers  0.0 to  1.0  ✓
    """
    return int(np.floor(lon))

def tile_name_from_index(lat_idx, lon_idx):
    """Build Copernicus tile name from SW corner indices."""
    lat_prefix = 'N' if lat_idx >= 0 else 'S'
    lon_prefix = 'E' if lon_idx >= 0 else 'W'
    return (f"Copernicus_DSM_COG_10_"
            f"{lat_prefix}{abs(lat_idx):02d}_00_"
            f"{lon_prefix}{abs(lon_idx):03d}_00_DEM")

# ============================================================
# DEM ACQUISITION
# ============================================================

def download_copernicus_tile(lat_idx, lon_idx, output_dir):
    """
    Download a single Copernicus GLO-30 tile by its SW corner indices.
    Returns local path or None if tile doesn't exist in dataset.
    """
    import boto3
    from botocore import UNSIGNED
    from botocore.config import Config

    tile_name   = tile_name_from_index(lat_idx, lon_idx)
    s3_key      = f"{tile_name}/{tile_name}.tif"
    output_path = os.path.join(output_dir, f"{tile_name}.tif")

    if os.path.exists(output_path):
        print(f"    Already exists: {tile_name}")
        return output_path

    s3     = boto3.client('s3', config=Config(signature_version=UNSIGNED))
    bucket = 'copernicus-dem-30m'

    # Use list_objects_v2 to check existence — avoids ClientError handling issues
    response = s3.list_objects_v2(Bucket=bucket, Prefix=s3_key, MaxKeys=1)
    if 'Contents' not in response:
        print(f"    Not found in dataset, skipping: {tile_name}")
        return None

    print(f"    Downloading {tile_name}...")
    s3.download_file(bucket, s3_key, output_path)
    print(f"    Saved: {output_path}")
    return output_path


def get_tiles_for_city(city_name, lat, lon, output_dir, buffer_deg=0.15):
    """
    Determine which 1°x1° tiles cover the city bounding box
    and download any that aren't already present.
    """
    lat_min = lat - buffer_deg
    lat_max = lat + buffer_deg
    lon_min = lon - buffer_deg
    lon_max = lon + buffer_deg

    lat_indices = set(range(lat_tile_index(lat_min),
                            lat_tile_index(lat_max) + 1))
    lon_indices = set(range(lon_tile_index(lon_min),
                            lon_tile_index(lon_max) + 1))

    print(f"  Tiles needed for {city_name}: "
          f"lat={sorted(lat_indices)} lon={sorted(lon_indices)}")

    paths = []
    for la in lat_indices:
        for lo in lon_indices:
            path = download_copernicus_tile(la, lo, output_dir)
            if path is not None:
                paths.append(path)

    if not paths:
        raise RuntimeError(
            f"No DEM tiles found for {city_name} at lat={lat}, lon={lon}")
    return paths


def merge_tiles(tile_paths, output_path):
    """Merge multiple tiles into one, or copy if only one tile."""
    if os.path.exists(output_path):
        print(f"    Merged DEM already exists: {output_path}")
        return output_path

    if len(tile_paths) == 1:
        shutil.copy2(tile_paths[0], output_path)
        print(f"    Single tile copied to: {output_path}")
        return output_path

    datasets        = [rasterio.open(p) for p in tile_paths]
    mosaic, transform = merge(datasets)
    profile         = datasets[0].profile
    profile.update({
        "height":    mosaic.shape[1],
        "width":     mosaic.shape[2],
        "transform": transform
    })
    with rasterio.open(output_path, 'w', **profile) as dst:
        dst.write(mosaic)
    for ds in datasets:
        ds.close()
    print(f"    Merged {len(tile_paths)} tiles → {output_path}")
    return output_path


def dem_path_for(dem_dir, city_name):
    return os.path.join(dem_dir, city_name, f"{city_name}_dem.tif")


def ensure_dem(dem_dir, city_name, lat, lon):
    city_dem_dir = os.path.join(dem_dir, city_name)
    os.makedirs(city_dem_dir, exist_ok=True)
    dem_path = dem_path_for(dem_dir, city_name)

    if os.path.exists(dem_path):
        # Validate bounds in a separate block so file is fully closed before any delete
        needs_redownload = False
        with rasterio.open(dem_path) as src:
            b = src.bounds
            if not (b.left <= lon <= b.right and b.bottom <= lat <= b.top):
                print(f"  WARNING: existing DEM for {city_name} does not "
                      f"cover ({lat}, {lon}) — bounds are {b}")
                print(f"  Deleting and re-downloading...")
                needs_redownload = True
            else:
                print(f"  DEM ready: {dem_path}")

        # File is now closed — safe to delete on Windows
        if needs_redownload:
            os.remove(dem_path)
        else:
            return dem_path

    print(f"  Fetching DEM for {city_name}...")
    tile_paths = get_tiles_for_city(city_name, lat, lon, city_dem_dir)
    return merge_tiles(tile_paths, dem_path)
