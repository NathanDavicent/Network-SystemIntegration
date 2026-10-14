# IoT-lösning: potentiometer via MQTT till REST-API

ESP32 läser en potentiometer och publicerar värdet som JSON via MQTT till en Mosquitto-broker. En Python-tjänst validerar och loggar datan och exponerar den via ett REST-API.

## Mappar

- `src/esp32/` ESP-IDF-projekt (öppna **denna** mapp, inte repots rot)
- `src/service/` Python-tjänst
- `broker/` exempelkonfig för Mosquitto
- `docs/` arkitektur, API, säkerhet, felsökning
- `tests/` enhetstester och testprotokoll

## Beroenden

ESP-IDF 5.4+ (testat på 6.0.1), Mosquitto 2.x, Python 3.11+, ESP32 med potentiometer (mitten-ben till GPIO34, ytterben till 3V3 och GND).

## Installation och konfiguration

1. **Broker:** skapa `C:\iot`, kopiera `broker/mosquitto.conf.example` till `C:\iot\mosquitto.conf`, och skapa användare:
```
   mosquitto_passwd -c C:\iot\passwd esp32
   mosquitto_passwd -b C:\iot\passwd service "<lösenord>"
```
2. **ESP32:** `cd src/esp32`, kör `idf.py menuconfig` och fyll i *IoT Configuration* (Wi-Fi SSID/lösenord, MQTT-URI `mqtt://<PC-IP>:1883`, MQTT-användare `esp32` och lösenord). Värdena sparas i `sdkconfig` (ignoreras av git).
3. **Tjänst:**
```
   cd src/service
   python -m venv .venv
   .venv\Scripts\python.exe -m pip install -r requirements.txt
```

## Starta

1. Broker (ett fönster, håll öppet; se till att ingen annan Mosquitto körs):
   `"C:\Program Files\mosquitto\mosquitto.exe" -c C:\iot\mosquitto.conf -v`
2. Tjänst:
```
   $env:MQTT_HOST="localhost"; $env:MQTT_USER="service"; $env:MQTT_PASS="<lösenord>"; $env:API_KEY="<nyckel>"
   python app.py
```
3. ESP32: `idf.py -p COM4 flash monitor` i `src/esp32`.

## Verifiera dataflödet

- Monitor visar `[MQTT] connected` och `[PUB] {...}`.
- `mosquitto_sub -h localhost -t "fastighet/#" -v -u service -P <lösenord>` visar JSON var 5:e sekund.
- `curl.exe http://localhost:5000/health` visar `received` som ökar.
- `curl.exe -H "X-API-Key: <nyckel>" http://localhost:5000/api/sensors` visar senaste värdet.

Tester: `cd src/service; python -m pytest ..\..\tests -v`

## Kända begränsningar

MQTT utan TLS (lösenord i klartext på LAN), ingen databas (bara senaste värde i minnet), Flasks utvecklingsserver, en enda broker utan redundans, brokerns IP är hårdkodad i konfigen. Se `docs/sakerhet.md`.
