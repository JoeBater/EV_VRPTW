import numpy as np
import pytest

from ev_vrptw_dataprep.config import Paths, metres_per_unit
from ev_vrptw_dataprep.geometry import (all_pairs_short_paths, compute_route,
                                        compute_slopes, prim_mst, sample_edge,
                                        xy_to_latlon)
from ev_vrptw_dataprep.pipeline import load_solomon

# A line 0-1-2-3 with a branch 1-4
POINTS = np.array([[0, 0], [1, 0], [2, 0], [3, 0], [1, 1]], dtype=float)


def test_prim_mst_is_spanning_tree():
    mst = prim_mst(POINTS)
    assert len(mst) == len(POINTS) - 1
    edges = {frozenset((u, v)) for u, v, _ in mst}
    assert edges == {frozenset(e) for e in [(0, 1), (1, 2), (2, 3), (1, 4)]}


def test_compute_route_excludes_endpoints():
    mst = prim_mst(POINTS)
    assert compute_route(mst, 5, 0, 3) == [1, 2]
    assert compute_route(mst, 5, 4, 3) == [1, 2]
    assert compute_route(mst, 5, 0, 1) == []      # direct neighbours


def test_short_paths_are_reversible():
    sp = all_pairs_short_paths(prim_mst(POINTS), 5)
    for i in sp:
        for j in sp[i]:
            assert sp[i][j] == sp[j][i][::-1]


def test_xy_to_latlon_maps_depot_exactly():
    lat, lon = xy_to_latlon(1750, 1750, 51.378, -2.3597, 1750, 1750)
    assert (lat, lon) == (51.378, -2.3597)


def test_sample_edge_includes_endpoints():
    s = sample_edge((0, 0), (250, 0), 100)
    assert s[0] == (0, 0) and s[-1] == (250, 0)


def test_compute_slopes_zero_run_is_flat():
    assert compute_slopes([0, 10, 10], [100, 0]) == [0.1, 0.0]


def test_metres_per_unit():
    assert metres_per_unit(5) == 50
    assert metres_per_unit(10) == 100


@pytest.mark.parametrize("benchmark", ["r101", "c107", "rc201"])
def test_short_paths_independent_of_scale(benchmark):
    """Justifies writing one short-paths file per benchmark."""
    path = Paths.default().instances_dir / f"{benchmark}.csv"
    if not path.exists():
        pytest.skip(f"{path} not present")
    xy = load_solomon(path)[["x", "y"]].values.astype(float)
    n = len(xy)
    reference = all_pairs_short_paths(prim_mst(xy), n)
    for scale in (5, 10):
        scaled = xy * metres_per_unit(scale)
        assert all_pairs_short_paths(prim_mst(scaled), n) == reference
