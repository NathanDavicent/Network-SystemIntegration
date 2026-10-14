# Säkerhet

## Risker och åtgärder

| # | Risk | Åtgärd |
|---|------|--------|
| 1 | Obehörig ansluter till MQTT-brokern och läser eller skickar data | `allow_anonymous false` med användare och lösenord (`esp32`, `service`) i Mosquitto-lösenordsfil |
| 2 | Obehörig läser sensordata via API:et | API-nyckel i headern `X-API-Key`, obehöriga anrop loggas och får 401 |
| 3 | Felaktig eller skadlig data matas in | Tjänsten validerar JSON och fält, ogiltiga meddelanden förkastas, loggas och räknas |
| 4 | Hemligheter läcker via versionshanteringen | Wi-Fi- och MQTT-uppgifter ligger i `sdkconfig` och miljövariabler, aldrig i git |

## Hantering av känslig konfiguration

- Wi-Fi SSID/lösenord och MQTT-uppgifter sätts via `idf.py menuconfig` och lagras i `src/esp32/sdkconfig`, som är i `.gitignore`.
- Tjänstens uppgifter (`MQTT_USER`, `MQTT_PASS`, `API_KEY`) läses från miljövariabler.
- `passwd` (brokerns lösenordsfil) och `*.key` ligger utanför repot eller i `.gitignore`. Bara `mosquitto.conf.example` checkas in.

## Kvarvarande begränsningar

- MQTT går över okrypterad port 1883, så lösenordet skickas i klartext på det lokala nätverket. Förbättring: TLS på port 8883 med CA-certifikat på ESP32.
- API:et använder HTTP och en delad nyckel, utan användarroller eller rate limiting.
- Flasks utvecklingsserver används, inte en produktionsserver.
- Lösenord i testmiljön är enkla och ska bytas före skarp drift.
