import numpy as np

# ============================================================
# VISUALISATION
# ============================================================

def visualise_instance(city_name, benchmark, scale,
                       coordinates, mst, edge_samples, png_path):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    import matplotlib.cm as cm
    import matplotlib.colors as mcolors

    all_samples    = []
    all_elevations = []
    for edge in edge_samples:
        all_samples.extend(edge["samples"])
        all_elevations.extend(
            edge.get("elevations", [np.nan] * len(edge["samples"])))

    all_elevations = np.array(all_elevations, dtype=float)
    valid = all_elevations[~np.isnan(all_elevations)]
    if len(valid) == 0:
        print("  No valid elevations to plot")
        return

    elev_min, elev_max = valid.min(), valid.max()
    norm = mcolors.Normalize(vmin=elev_min, vmax=elev_max)
    cmap = cm.terrain

    fig, ax = plt.subplots(figsize=(12, 12))

    for u, v, _ in mst:
        ax.plot([coordinates[u][0], coordinates[v][0]],
                [coordinates[u][1], coordinates[v][1]],
                color='blue', linewidth=1.5, alpha=0.4)

    ax.scatter(coordinates[:, 0], coordinates[:, 1],
               color='black', s=60, zorder=5, label='Sites')

    xs = [s[0] for s in all_samples]
    ys = [s[1] for s in all_samples]
    sc = ax.scatter(xs, ys, c=all_elevations, cmap=cmap, norm=norm,
                    s=15, alpha=0.9, zorder=4, label='Substeps (elevation)')

    cbar = fig.colorbar(sc, ax=ax, fraction=0.03, pad=0.02)
    cbar.set_label("Elevation (m)", fontsize=12)

    ax.set_title(f"{city_name} | {benchmark} | scale={scale} | "
                 f"elev {elev_min:.0f}–{elev_max:.0f}m", fontsize=13)
    ax.set_xlabel("X (scaled, m)")
    ax.set_ylabel("Y (scaled, m)")
    ax.legend(loc='upper right')
    ax.grid(True, alpha=0.3)
    plt.tight_layout()

    plt.savefig(png_path, dpi=300, bbox_inches='tight', format='png')
    print(f"  Saved: {png_path}")
    plt.close(fig)  # free memory — important for 20x2x15 runs
