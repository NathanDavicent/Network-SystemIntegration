# IoT-lösning: potentiometer via MQTT till REST-API

ESP32 läser en potentiometer och publicerar värdet som JSON via MQTT (TLS) till en Mosquitto-broker. En Python-tjänst validerar och loggar datan och exponerar den via ett REST-API.

## Mappar

- `src/esp32/` ESP-IDF-projekt (öppna **denna** mapp, inte repots rot)
- `src/service/` Python-tjänst
- `broker/` exempelkonfig för Mosquitto
- `docs/` arkitektur, API, säkerhet, felsökning
- `tests/` enhetstester och testprotokoll

## Beroenden

ESP-IDF 5.4+ (testat på 6.0.1), Mosquitto 2.x, Python 3.11+, openssl (följer med Git for Windows), ESP32 med potentiometer (mitten-ben till GPIO34, ytterben till 3V3 och GND).

## Installation och konfiguration

1. **Certifikat** (byt `<PC-IP>` mot brokerns IP, se `ipconfig`):
```
   mkdir C:\iot\certs; cd C:\iot\certs
   $ssl = "C:\Program Files\Git\usr\bin\openssl.exe"; $ip = "<PC-IP>"
   & $ssl genrsa -out ca.key 2048
   & $ssl req -x509 -new -key ca.key -sha256 -days 365 -subj "/CN=IoT-CA" -out ca.crt
   & $ssl genrsa -out server.key 2048
   & $ssl req -new -key server.key -subj "/CN=$ip" -out server.csr
   Set-Content san.ext "subjectAltName=IP:$ip"
   & $ssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out server.crt -days 365 -sha256 -extfile san.ext
   copy ca.crt <repo>\src\esp32\main\ca.crt
```
   Privata nycklar (`*.key`) checkas aldrig in. Bara `ca.crt` ligger i repot.

2. **Broker:** kopiera `broker/mosquitto.conf.example` till `C:\iot\mosquitto.conf` och skapa användare:
```
   mosquitto_passwd -c C:\iot\passwd esp32
   mosquitto_passwd -b C:\iot\passwd service "<lösenord>"
```
   Öppna port 8883 i brandväggen (administratör):
   `New-NetFirewallRule -DisplayName "MQTT TLS 8883" -Direction Inbound -Protocol TCP -LocalPort 8883 -Action Allow -Profile Any`

3. **ESP32:** `cd src/esp32`, kör `idf.py menuconfig` och fyll i *IoT Configuration*: Wi-Fi SSID/lösenord, MQTT-URI `mqtts://<PC-IP>:8883`, MQTT-användare `esp32` och lösenord. Värdena sparas i `sdkconfig` (ignoreras av git).

4. **Tjänst:**
```
   cd src/service
   python -m venv .venv
   .venv\Scripts\python.exe -m pip install -r requirements.txt
```

## Starta

1. Broker (eget fönster, håll öppet; se till att ingen annan Mosquitto körs):
   `"C:\Program Files\mosquitto\mosquitto.exe" -c C:\iot\mosquitto.conf -v`
2. Tjänst:
```
   $env:MQTT_HOST="localhost"; $env:MQTT_USER="service"; $env:MQTT_PASS="<lösenord>"; $env:API_KEY="<nyckel>"
   .venv\Scripts\python.exe app.py
```
3. ESP32 (ESP-IDF-terminal): `idf.py -p COM4 flash monitor` i `src/esp32`.

## Verifiera dataflödet

- Monitor visar `[MQTT] connected` och `[PUB] {...}`.
- Brokerloggen visar en anslutning på port 8883 med `TLSv1.2/1.3`.
- `mosquitto_sub -h <PC-IP> -p 8883 --cafile C:\iot\certs\ca.crt -u service -P <lösenord> -t "fastighet/#" -v` visar JSON var 5:e sekund.
- `curl.exe http://localhost:5000/health` visar `received` som ökar.
- `curl.exe -H "X-API-Key: <nyckel>" http://localhost:5000/api/sensors` visar senaste värdet.

Tester: `cd src/service; .venv\Scripts\python.exe -m pytest ..\..\tests -v`

## Kända begränsningar

Självsignerat certifikat (giltigt 365 dagar, knutet till brokerns IP), ingen databas (bara senaste värde i minnet), API över HTTP med delad nyckel, Flasks utvecklingsserver, en enda broker utan redundans. Se `docs/sakerhet.md` och `docs/felsokning.md`.