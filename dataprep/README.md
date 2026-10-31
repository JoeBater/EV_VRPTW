# dataprep

Python pipeline that builds the solver's inputs in [`../data`](../data).

> Being migrated from a separate project. This README will be filled in as the code arrives.

## Pipeline stages (planned)

1. **Scale**: scale the Solomon coordinates to a city-sized footprint (`scale` factor)
2. **Geocode**: map scaled coordinates to latitude and longitude around each city centre
3. **Elevation**: look up elevation for every site (Copernicus DEM), cached locally
4. **Short paths**: compute paths between site pairs → `<benchmark>_short_paths.txt`
5. **Weather**: download hourly Open-Meteo data per city → `<City>-weather.csv`
6. **Check**: validate and write `<City>_<scale>_<benchmark>.csv`

Planned layout:

```
dataprep/
├── pyproject.toml
├── src/ev_vrptw_dataprep/
├── notebooks/      Exploratory notebooks
└── tests/
```
