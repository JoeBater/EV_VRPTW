import os
from dataclasses import dataclass
from pathlib import Path

# ============================================================
# CONFIGURATION
# ============================================================

# dataprep/src/ev_vrptw_dataprep/config.py -> repository root
REPO_ROOT = Path(__file__).resolve().parents[3]

SUBSTEP_INTERVAL = 100   # metres between elevation sample points
SOLOMON_GRID     = 100   # Solomon coordinates span 0-100 units


def metres_per_unit(scale_km):
    """
    Solomon's 100x100 grid mapped onto a scale_km x scale_km square.
    scale=5 -> 1 unit = 50 m, scale=10 -> 1 unit = 100 m.
    """
    return scale_km * (1000 // SOLOMON_GRID)


@dataclass
class Paths:
    instances_dir:   Path   # raw Solomon CSVs     - read only
    cities_file:     Path   # city table           - read only
    dem_dir:         Path   # Copernicus TIFFs     - download cache
    processed_dir:   Path   # all generated output

    @classmethod
    def default(cls):
        data = Path(os.environ.get("EV_VRPTW_DATA", REPO_ROOT / "data"))
        return cls(
            instances_dir = data / "raw" / "solomon",
            cities_file   = data / "raw" / "cities.csv",
            dem_dir       = data / "dem",
            processed_dir = data / "processed",
        )

    @property
    def scaled_instances_dir(self):
        return self.processed_dir / "instances"

    def city_instances_dir(self, city):
        """Instances are filed by city: instances/<City>/<City>_<scale>_<benchmark>.csv"""
        return self.scaled_instances_dir / city

    @property
    def extended_dir(self):
        """Solomon layouts with stores added (extend.py), same CSV format as raw/solomon"""
        return self.instances_dir.parent / "extended"

    @property
    def mst_dir(self):
        """Tree edge lists for networks that are not a fresh Prim MST (extend.py)"""
        return self.processed_dir / "mst"

    @property
    def short_paths_dir(self):
        return self.processed_dir / "short_paths"

    @property
    def edge_samples_dir(self):
        return self.processed_dir / "edge_samples"

    @property
    def edge_profiles_dir(self):
        return self.processed_dir / "edge_profiles"

    @property
    def plots_dir(self):
        return self.processed_dir / "plots"

    @property
    def weather_dir(self):
        return self.processed_dir.parent / "weather" / "open-meteo"
