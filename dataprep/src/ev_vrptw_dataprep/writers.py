import json
import os

import numpy as np

# ============================================================
# OUTPUT WRITERS
# ============================================================

def write_scaled_instance(df, output_path):
    df.to_csv(output_path, index=False)
    print(f"  Written: {output_path}")


def write_short_paths(short_paths, output_path):
    """
    Format: 'from to: step1 step2 ... stepN'
    Matches C++ MST::readSpanningTree expected format.
    Direct MST neighbours have empty step list.
    """
    with open(output_path, 'w') as f:
        for from_site, destinations in short_paths.items():
            for to_site, steps in destinations.items():
                if from_site == to_site:
                    continue
                steps_str = ' '.join(str(s) for s in steps)
                f.write(f"{from_site} {to_site}: {steps_str}\n")
    print(f"  Written: {output_path}")


def write_edge_profiles(edge_samples, output_path):
    """
    Format: CSV 'start,end,length,dz', one row per ~100 m segment of each
    MST edge, in order from start to end. Matches C++ MST::readEdgeProfiles.
    The solver walks the segments in reverse (negating dz) for end -> start.
    """
    with open(output_path, 'w') as f:
        f.write("start,end,length,dz\n")
        for edge in edge_samples:
            for length, e0, e1 in zip(edge["distances"],
                                      edge["elevations"][:-1], edge["elevations"][1:]):
                f.write(f"{edge['start']},{edge['end']},{length:.2f},{e1 - e0:.3f}\n")
    print(f"  Written: {output_path}")


def write_edge_samples(edge_samples, output_path):
    def clean(obj):
        if isinstance(obj, (np.floating, np.float32, np.float64)):
            return float(obj)
        if isinstance(obj, np.integer):
            return int(obj)
        if isinstance(obj, (list, tuple)):
            return [clean(i) for i in obj]
        return obj

    serialisable = [{k: clean(v) for k, v in edge.items()}
                    for edge in edge_samples]
    with open(output_path, 'w') as f:
        json.dump(serialisable, f, indent=2)
    print(f"  Written: {output_path}")
