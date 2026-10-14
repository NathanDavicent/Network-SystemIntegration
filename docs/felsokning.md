# Felsökning

Tre fel har införts eller uppstått avsiktligt/oavsiktligt under utvecklingen och analyserats.

## Fel 1: ESP32 saknar MQTT-inloggning

1. **Observerat symptom:** ESP32 skriver `Connection refused, not authorized` och `[MQTT] error`. Inga mätvärden når tjänsten.
2. **Hur felet identifierades:** Felet syntes direkt i ESP32:ns monitor. Brokerns logg visade samma sak från andra hållet.
3. **Verktyg/loggar:** ESP-IDF monitor, Mosquitto-loggen (`mosquitto -v`), `Select-String` i `sdkconfig`.
4. **Orsak:** Brokern kräver användarnamn och lösenord (`allow_anonymous false`), men `CONFIG_MQTT_USERNAME` och `CONFIG_MQTT_PASSWORD` var tomma i `sdkconfig`.
5. **Åtgärd:** Värdena sattes i `sdkconfig`, och firmware byggdes och flashades om.
6. **Verifiering:** Monitor visade `[MQTT] connected` följt av `[PUB]`-rader, och Mosquitto loggade `New client connected ... u'esp32'`.

## Fel 2: Brokern körde inte

1. **Observerat symptom:** `esp-tls: select() timeout`, `Failed to open a new connection`, `[MQTT] error`. Wi-Fi var ok men `mqtt=no`.
2. **Hur felet identifierades:** `Test-NetConnection 192.168.50.254 -Port 1883` gav `TcpTestSucceeded : False`.
3. **Verktyg/loggar:** ESP-IDF monitor, `Test-NetConnection`, `netstat -ano | findstr 1883`.
4. **Orsak:** Mosquitto-fönstret var stängt, så ingen lyssnade på port 1883. En timeout (inte ett avslag) betyder att ingenting svarar.
5. **Åtgärd:** Brokern startades med `mosquitto.exe -c C:\iot\mosquitto.conf -v`.
6. **Verifiering:** Mosquitto loggade `New connection from 192.168.50.247` och ESP32 ansluter automatiskt igen.

## Fel 3: Två brokers körde samtidigt

1. **Observerat symptom:** Python-tjänsten loggade `MQTT connected`, ESP32 publicerade, men `/health` visade `received: 0` och `/api/sensors` returnerade `[]`.
2. **Hur felet identifierades:** Brokerns logg visade bara `Sending PUBACK` men aldrig `Sending PUBLISH` till prenumeranten. `mosquitto_sub` fick inga meddelanden.
3. **Verktyg/loggar:** `Get-Process mosquitto`, `netstat -ano | findstr 1883`, Mosquitto-loggen.
4. **Orsak:** Windows-tjänsten Mosquitto (PID 1016) och en manuellt startad broker (PID 4700) lyssnade båda på 1883. ESP32 var ansluten till den ena och tjänsten till den andra.
5. **Åtgärd:** `Stop-Service mosquitto`, alla Mosquitto-processer avslutades och en enda broker startades. Lösenordet för användaren `service` sattes om.
6. **Verifiering:** `netstat` visade en enda lyssnande PID, loggen visade `Sending PUBLISH`, och `/health` visade `received` som räknade upp.

## Extra: ogiltig JSON

`mosquitto_pub ... -m "not json"` gav `ERROR Invalid JSON` i tjänstloggen och ökade `invalid` i `/health`.
