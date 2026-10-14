# API

Bas-URL: `http://<värd>:5000` (utveckling: `http://localhost:5000`). Alla svar är JSON.

## Autentisering

Alla endpoints utom `/health` kräver headern `X-API-Key: <nyckel>`. Nyckeln sätts i miljövariabeln `API_KEY` när tjänsten startas.

## GET /health

Övervakningsmått. Kräver ingen nyckel.

Exempel:
    curl.exe http://localhost:5000/health

Svar 200:
    {"received": 5, "invalid": 1, "seconds_since_last_message": 1.0}

- `received`: antal giltiga meddelanden
- `invalid`: antal förkastade meddelanden (ogiltig JSON eller valideringsfel)
- `seconds_since_last_message`: sekunder sedan senaste giltiga meddelande, `null` om inget mottagits

## GET /api/sensors

Returnerar senaste mätvärdet för varje sensor.

Exempel:
    curl.exe -H "X-API-Key: <nyckel>" http://localhost:5000/api/sensors

Svar 200:
    [{"sensorId": "room-a-pot-01", "timestamp": "2026-10-14T12:55:41+02:00", "value": 4.8, "unit": "%"}]

Tom lista `[]` om inga data mottagits än.

## GET /api/sensors/{sensorId}/latest

Parameter: `sensorId` i sökvägen, t.ex. `room-a-pot-01`.

Exempel:
    curl.exe -H "X-API-Key: <nyckel>" http://localhost:5000/api/sensors/room-a-pot-01/latest

Svar 200: ett sensorobjekt som ovan.

## Felkontrakt

Fel returneras som `{"error": "<kod>", "message": "<text>"}`.

| Status | error | När |
|--------|-------|-----|
| 401 | `unauthorized` | `X-API-Key` saknas eller är fel |
| 404 | `not_found` | Okänt `sensorId` |

## Datakontrakt (MQTT-meddelande)

Topic: `fastighet/<rum>/<sensor>/data`

    {"sensorId": "room-a-pot-01", "timestamp": "2026-10-14T12:55:41+02:00", "value": 4.8, "unit": "%"}

Valideras av tjänsten: `sensorId` och `unit` är strängar, `value` är ett tal, `timestamp` är ISO 8601.
