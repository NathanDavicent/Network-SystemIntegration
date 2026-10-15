# Säkerhet

## Risker och åtgärder

| # | Risk | Åtgärd |
|---|------|--------|
| 1 | Obehörig ansluter till MQTT-brokern och läser eller skickar data | `allow_anonymous false` med användare och lösenord (`esp32`, `service`) i Mosquitto-lösenordsfil |
| 2 | Avlyssning av MQTT-trafik (inklusive lösenord) på nätverket | TLS på port 8883. ESP32 verifierar brokern med ett inbäddat CA-certifikat. Port 1883 lyssnar bara på localhost |
| 3 | Obehörig läser sensordata via API:et | API-nyckel i headern `X-API-Key`, obehöriga anrop loggas och får 401 |
| 4 | Felaktig eller skadlig data matas in | Tjänsten validerar JSON och fält, ogiltiga meddelanden förkastas, loggas och räknas |
| 5 | Hemligheter läcker via versionshanteringen | Wi-Fi- och MQTT-uppgifter ligger i `sdkconfig` och miljövariabler, privata nycklar och `passwd` ligger utanför repot. Bara `ca.crt` (publikt) checkas in |

## Hantering av känslig konfiguration

- Wi-Fi SSID/lösenord och MQTT-uppgifter sätts via `idf.py menuconfig` och lagras i `src/esp32/sdkconfig`, som är i `.gitignore`.
- Tjänstens uppgifter (`MQTT_USER`, `MQTT_PASS`, `API_KEY`) läses från miljövariabler.
- `passwd` och `*.key` ligger i `C:\iot` och checkas aldrig in.

## Kvarvarande begränsningar

- Det självsignerade CA-certifikatet gäller 365 dagar och måste då skapas om.
- Certifikatet är knutet till brokerns IP-adress (`192.168.50.254`). Ändras IP:n måste certifikatet utfärdas på nytt.
- Privata nycklar ligger okrypterade på brokerdatorn.
- Lokala lyssnaren på 1883 (tjänst till broker) är okrypterad, men nås bara från samma dator.
- API:et använder HTTP och en delad nyckel, utan användarroller eller rate limiting.
- Flasks utvecklingsserver används, inte en produktionsserver.
- Testlösenord är enkla och ska bytas före skarp drift.