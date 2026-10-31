import numpy as np

# ============================================================
# GEOMETRY
# ============================================================

def euclidean_distance(p1, p2):
    return np.sqrt((p1[0] - p2[0])**2 + (p1[1] - p2[1])**2)


def prim_mst(points):
    n        = len(points)
    visited  = [False] * n
    min_edge = [(None, float('inf'))] * n
    min_edge[0] = (None, 0.0)
    mst = []
    for _ in range(n):
        u = min((i for i in range(n) if not visited[i]),
                key=lambda i: min_edge[i][1])
        visited[u] = True
        if min_edge[u][0] is not None:
            mst.append((min_edge[u][0], u, min_edge[u][1]))
        for v in range(n):
            if not visited[v]:
                w = euclidean_distance(points[u], points[v])
                if w < min_edge[v][1]:
                    min_edge[v] = (u, w)
    return mst


def compute_route(mst, n, start, end):
    """
    DFS path on MST from start to end.
    Returns intermediate nodes only — excludes destination,
    consistent with C++ MST::getSteps convention.
    """
    graph = {i: [] for i in range(n)}
    for u, v, _ in mst:
        graph[u].append(v)
        graph[v].append(u)

    def dfs(current, target, visited, path):
        if current == target:
            return path
        visited.add(current)
        for nb in graph[current]:
            if nb not in visited:
                result = dfs(nb, target, visited, path + [nb])
                if result:
                    return result
        return None

    route = dfs(start, end, set(), [])
    return route[:-1] if route else []


def all_pairs_short_paths(mst, n):
    """Intermediate MST nodes for every ordered pair (i, j), i != j."""
    short_paths = {i: {} for i in range(n)}
    for i in range(n):
        for j in range(n):
            if i != j:
                short_paths[i][j] = compute_route(mst, n, i, j)
    return short_paths


def sample_edge(A, B, interval):
    """Interpolate points along edge A→B at ~interval metre spacing."""
    dx, dy = B[0] - A[0], B[1] - A[1]
    dist   = (dx**2 + dy**2)**0.5
    steps  = max(1, int(dist // interval))
    return [(A[0] + i * dx / steps, A[1] + i * dy / steps)
            for i in range(steps + 1)]


def xy_to_latlon(x, y, depot_lat, depot_lon, depot_x, depot_y):
    """
    Convert scaled (x,y) metres to lat/lon using depot as geographic origin.
    Subtracts depot scaled coords before projecting so depot maps to
    (depot_lat, depot_lon) exactly.
    """
    dx         = x - depot_x
    dy         = y - depot_y
    lat_offset = dy / 111320.0
    lon_offset = dx / (111320.0 * np.cos(np.radians(depot_lat)))
    return depot_lat + lat_offset, depot_lon + lon_offset


def compute_horizontal_distances(samples):
    return [
        np.hypot(samples[i+1][0] - samples[i][0],
                 samples[i+1][1] - samples[i][1])
        for i in range(len(samples) - 1)
    ]


def compute_slopes(elevations, distances):
    rise = np.diff(np.array(elevations))
    run  = np.array(distances)
    with np.errstate(divide='ignore', invalid='ignore'):
        slopes = np.where(run > 0, rise / run, 0.0)
    return slopes.tolist()
