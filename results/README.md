# results

The solver writes its run outputs here. Everything in this folder except this README is
git-ignored.

Layout:

```
results/
├── searchProgress.csv                       LNS cost per iteration (latest run)
└── solutions/flatland_<scale>/<benchmark>/
    ├── route_log_<month>.txt                exp1 solution (exp0 reads this back in)
    ├── detailed_route_log_<month>[_<day>].csv
    └── <City>/detailed_route_log_<month>_<day>.csv   the solution re-simulated in a city, every day of the year
```

`detailed_route_log_*.csv` is the step-by-step simulation: time, position, state of charge
and weather. See [solver/README.md](../solver/README.md) for how to point outputs elsewhere.
