import json, logging, os, time
from datetime import datetime
import paho.mqtt.client as mqtt
from flask import Flask, jsonify, request, abort

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s %(levelname)s %(message)s",
    handlers=[logging.StreamHandler(), logging.FileHandler("service.log")],
)
log = logging.getLogger("iot")

MQTT_HOST = os.environ.get("MQTT_HOST", "localhost")
MQTT_PORT = int(os.environ.get("MQTT_PORT", 1883))
MQTT_USER = os.environ.get("MQTT_USER", "")
MQTT_PASS = os.environ.get("MQTT_PASS", "")
API_KEY = os.environ.get("API_KEY", "")
TOPIC = "fastighet/+/+/data"

state = {"latest": {}, "received": 0, "invalid": 0, "last_seen": None}


def valid(d):
    try:
        datetime.fromisoformat(d["timestamp"])
        return (isinstance(d["sensorId"], str)
                and isinstance(d["value"], (int, float))
                and isinstance(d["unit"], str))
    except (KeyError, ValueError, TypeError, AttributeError):
        return False


def on_connect(c, userdata, flags, reason_code, properties=None):
    if reason_code != 0:
        log.error("MQTT connection refused (rc=%s)", reason_code)
        return
    log.info("MQTT connected (rc=%s)", reason_code)
    c.subscribe(TOPIC, qos=1)


def on_disconnect(c, userdata, flags, reason_code, properties=None):
    log.warning("MQTT disconnected (rc=%s), reconnecting automatically", reason_code)


def on_message(c, userdata, msg):
    try:
        d = json.loads(msg.payload)
    except json.JSONDecodeError:
        state["invalid"] += 1
        log.error("Invalid JSON on %s: %r", msg.topic, msg.payload[:80])
        return
    if not valid(d):
        state["invalid"] += 1
        log.error("Validation error on %s: %s", msg.topic, d)
        return
    state["latest"][d["sensorId"]] = d
    state["received"] += 1
    state["last_seen"] = time.time()
    log.info("Received %s", d)


client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.on_connect = on_connect
client.on_disconnect = on_disconnect
client.on_message = on_message
if MQTT_USER:
    client.username_pw_set(MQTT_USER, MQTT_PASS)
client.reconnect_delay_set(1, 30)
client.connect_async(MQTT_HOST, MQTT_PORT)
client.loop_start()

app = Flask(__name__)


@app.before_request
def auth():
    if request.path == "/health" or not API_KEY:
        return
    if request.headers.get("X-API-Key") != API_KEY:
        log.warning("Unauthorized request from %s", request.remote_addr)
        abort(401)


@app.errorhandler(401)
def unauthorized(e):
    return jsonify(error="unauthorized", message="Missing or wrong X-API-Key"), 401


@app.get("/health")
def health():
    age = None if state["last_seen"] is None else round(time.time() - state["last_seen"], 1)
    return jsonify(received=state["received"], invalid=state["invalid"],
                   seconds_since_last_message=age)


@app.get("/api/sensors")
def sensors():
    return jsonify(list(state["latest"].values()))


@app.get("/api/sensors/<sid>/latest")
def latest(sid):
    if sid not in state["latest"]:
        return jsonify(error="not_found", message=f"Unknown sensor {sid}"), 404
    return jsonify(state["latest"][sid])


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)