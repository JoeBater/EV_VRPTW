from ev_vrptw_dataprep.weather import COLUMNS, to_frame

PAYLOAD = {
    "latitude": 51.35, "longitude": -2.44, "elevation": 25.0,
    "hourly": {
        "time":               ["2024-02-29T23:00", "2024-03-01T00:00"],
        "temperature_2m":     [6.6, 6.8],
        "dew_point_2m":       [4.1, 3.9],
        "rain":               [0.0, 0.4],
        "wind_speed_10m":     [34.7, 34.7],
        "wind_direction_10m": [266, 264],
        "wind_gusts_10m":     [60.1, 58.0],
        "is_day":             [0, 1],
    },
}


def test_to_frame_matches_solver_layout():
    df = to_frame(PAYLOAD)
    assert list(df.columns) == COLUMNS
    # Instance::readWeatherFile reads these columns by position
    assert list(df.columns[[0, 1, 2, 3, 5]]) == ["month", "day", "hour", "temperature_2m", "rain"]


def test_to_frame_splits_timestamps():
    df = to_frame(PAYLOAD)
    assert df[["month", "day", "hour"]].values.tolist() == [[2, 29, 23], [3, 1, 0]]
    assert df["is_day"].tolist() == [0, 1]
