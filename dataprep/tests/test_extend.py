"""Extended layouts (extend.py) keep their base intact and stay a feasible tree."""
import math

import numpy as np
import pytest

from ev_vrptw_dataprep.config import Paths
from ev_vrptw_dataprep.geometry import prim_mst
from ev_vrptw_dataprep.pipeline import load_solomon, read_tree

PATHS = Paths.default()
EXTENDED = sorted(p.stem for p in PATHS.extended_dir.glob("*.csv"))


@pytest.mark.skipif(not EXTENDED, reason="no extended instances generated")
@pytest.mark.parametrize("name", EXTENDED)
def test_extended_instance(name):
    base_name = name.rsplit("_", 1)[0]
    ext  = load_solomon(PATHS.extended_dir / f"{name}.csv")
    base = load_solomon(PATHS.instances_dir / f"{base_name}.csv")
    n_base = len(base)

    # the base sites are untouched and the new ones numbered after them
    assert ext.iloc[:n_base].equals(base)
    assert list(ext.id) == list(range(1, len(ext) + 1))

    # the network is the base MST plus one spur per new store, and a single tree
    edges = read_tree(name, PATHS)
    assert len(edges) == len(ext) - 1
    base_mst = {frozenset((u, v)) for u, v, _ in prim_mst(base[["x", "y"]].values.astype(float))}
    assert base_mst <= {frozenset(e) for e in edges}
    seen = set(range(n_base))
    for u, v in edges[n_base - 1:]:          # spurs, in the order the stores were added
        assert u in seen and v == len(seen)
        seen.add(v)

    # every new store can be served inside its window and the vehicle can get home
    depot = ext.iloc[0]
    for s in ext.iloc[n_base:].itertuples():
        t = math.hypot(s.x - depot.x, s.y - depot.y)
        start = max(t, s.ready_time)
        assert s.ready_time + s.service_time <= s.due_time
        assert start + s.service_time <= s.due_time
        assert start + s.service_time + t <= depot.due_time
