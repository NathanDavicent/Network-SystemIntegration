import os
import sys

# Make the service importable and avoid needing real MQTT settings
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "src", "service"))
os.environ.setdefault("MQTT_HOST", "localhost")

from app import valid, app  # noqa: E402

GOOD = {
    "sensorId": "room-a-pot-01",
    "timestamp": "2026-10-14T12:55:41+02:00",
    "value": 4.8,
    "unit": "%",
}


def test_valid_message():
    assert valid(GOOD)


def test_missing_field():
    bad = dict(GOOD)
    del bad["value"]
    assert not valid(bad)


def test_bad_timestamp():
    assert not valid({**GOOD, "timestamp": "yesterday"})


def test_value_wrong_type():
    assert not valid({**GOOD, "value": "high"})


def test_not_a_dict():
    assert not valid("just a string")


def test_health_endpoint_needs_no_key():
    client = app.test_client()
    r = client.get("/health")
    assert r.status_code == 200
    assert "received" in r.get_json()


def test_unknown_sensor_returns_404():
    client = app.test_client()
    r = client.get("/api/sensors/does-not-exist/latest")
    assert r.status_code == 404
    assert r.get_json()["error"] == "not_found"