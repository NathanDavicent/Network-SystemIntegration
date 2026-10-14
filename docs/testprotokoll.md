# Testprotokoll

Miljö: ESP32 + potentiometer (GPIO34), Mosquitto 2.1.2 på Windows, Python-tjänst på port 5000.

| # | Test | Steg | Förväntat resultat | Resultat |
|---|------|------|--------------------|----------|
| 1 | Sensor till broker | Vrid potentiometern och kör `mosquitto_sub -h localhost -t "fastighet/#" -v -u service -P <lösenord>` | JSON-rad var 5:e sekund, `value` ändras när potentiometern vrids | OK |
| 2 | Broker till tjänst | Starta `python app.py` i `src/service` | `MQTT connected (rc=Success)` och `Received {...}` i loggen | OK |
| 3 | API med nyckel | `curl.exe -H "X-API-Key: <nyckel>" http://localhost:5000/api/sensors` | Senaste mätvärdet som JSON | OK |
| 4 | Övervakning | `curl.exe http://localhost:5000/health` | `received` ökar, `seconds_since_last_message` under 10 | OK |
| 5 | Okänd sensor | `curl.exe -H "X-API-Key: <nyckel>" http://localhost:5000/api/sensors/finns-inte/latest` | 404 `not_found` | OK |
| 6 | Ogiltig JSON | `mosquitto_pub -h localhost -t "fastighet/test/x/data" -m "not json" -u esp32 -P <lösenord>` | `ERROR Invalid JSON` i loggen, `invalid` ökar i `/health` | OK |
| 7 | Fel API-nyckel | `curl.exe http://localhost:5000/api/sensors` (utan nyckel) | 401 `unauthorized` | OK |
| 8 | Anonym MQTT nekas | `mosquitto_pub -h localhost -t test -m hej` utan inloggning | `Connection Refused: not authorised` | OK |
| 9 | Återanslutning | Stäng av brokern en stund och starta den igen | ESP32 loggar `MQTT disconnected`, ansluter automatiskt igen och fortsätter publicera | OK |
| 10 | Enhetstester | `python -m pytest ..\..\tests -v` i `src/service` | 7 passed | OK (7 passed) |

## Anmärkningar

- Fel som uppstod under utvecklingen (saknad MQTT-inloggning, broker nere, två brokers) beskrivs i `docs/felsokning.md`.
- Test 9 observerades när ESP32 tappade anslutningen kort och återanslöt själv.