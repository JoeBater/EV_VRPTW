# results

The solver writes its run outputs here. Everything in this folder except this README is
git-ignored.

Expected contents:

- `route_log_*.txt`: final routes (and the input for re-simulation)
- `detailed_route_log_*.csv`: step-by-step simulation (time, position, state-of-charge, weather)
- `searchProgress.csv`: LNS cost per iteration
- `video/best/`, `video/new/`: per-iteration route snapshots for animation
