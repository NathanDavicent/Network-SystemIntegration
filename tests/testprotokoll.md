# Testprotokoll

| # | Test | Steg | Förväntat resultat | Resultat |
|---|------|------|--------------------|----------|
| 1 | Sensor till broker | Vrid potentiometern, kör `mosquitto_sub -t "fastighet/#" -u service -P ...` | JSON-rad var 5:e sekund, value ändras | OK |
| 2 | Broker till tjänst | Starta `python app.py` | `MQTT connected` och `Received {...}` i loggen | OK |
| 3 | API | `curl.exe -H "X-API-Key: ..." http://localhost:5000/api/sensors` | Senaste mätvärdet som JSON | OK |
| 4 | Ogiltig JSON | `mosquitto_pub ... -m "not json"` | `ERROR Invalid JSON`, `invalid` ökar i `/health` | OK |
| 5 | Fel API-nyckel | `curl.exe http://localhost:5000/api/sensors` | 401 `unauthorized` | OK |
| 6 | Anonym MQTT nekas | `mosquitto_pub -t test -m hej` utan inloggning | `Connection Refused: not authorised` | OK |