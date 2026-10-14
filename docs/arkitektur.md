# Arkitektur

## Översikt

```
Potentiometer (GPIO34)
        │ analog
        ▼
     ESP32 ──── MQTT (TCP 1883, Wi-Fi) ───▶ Mosquitto-broker (PC)
                                                  │ MQTT (localhost:1883)
                                                  ▼
                                        Python-tjänst (Flask + paho-mqtt)
                                                  │ HTTP/REST (TCP 5000)
                                                  ▼
                                          Klient (curl, andra system)
```

## Komponenter

| Komponent | Ansvar | Teknik |
|-----------|--------|--------|
| ESP32 + potentiometer | Läser ADC, bygger JSON, publicerar var 5:e sekund, synkar tid via NTP | ESP-IDF, C++ |
| Mosquitto | Tar emot och vidarebefordrar meddelanden, kräver inloggning | Mosquitto 2.x |
| Python-tjänst | Prenumererar, validerar, loggar, räknar mått, serverar REST-API | Flask, paho-mqtt |

## Nätverk

- Broker: PC:ns IP på Wi-Fi/LAN (i testet `192.168.50.254`), port **1883**.
- Tjänsten ansluter till brokern via `localhost:1883`.
- API: port **5000**, HTTP.
- ESP32 får brokerns adress via `CONFIG_MQTT_BROKER_URI` i menuconfig.

## Kommunikationsmodell och topics

Publish/subscribe. ESP32 publicerar (QoS 1), tjänsten prenumererar på `fastighet/+/+/data`.

Topic för sensorn: `fastighet/rum-a/pot-01/data`

## Dataformat

```json
{"sensorId": "room-a-pot-01", "timestamp": "2026-10-14T12:55:41+02:00", "value": 4.8, "unit": "%"}
```

Samma kontrakt används av ESP32, valideringen i tjänsten och API:et.

## Återanslutning och kommunikationsfel

- ESP32: Wi-Fi försöker ansluta igen vid frånkoppling, och MQTT-klienten återansluter automatiskt. Mätvärden skickas bara när MQTT och tidssynk är klara.
- Tjänsten: `reconnect_delay_set(1, 30)` ger återanslutning med backoff. Avslag och frånkopplingar loggas.
- Ogiltig data förkastas, loggas och räknas i `/health`.

## Varför MQTT

Lättviktigt protokoll för små enheter med litet overhead, publish/subscribe kopplar loss sensor och konsument, QoS 1 ger leveransgaranti, och Last Will/keep-alive gör det lätt att upptäcka bortfall. HTTP hade krävt att tjänsten alltid nås av enheten och är tyngre per meddelande.

## Övervakning och loggning

`GET /health` visar antal mottagna och ogiltiga meddelanden samt sekunder sedan senaste meddelande. Tjänsten loggar anslutningar, mottagna data, valideringsfel och obehöriga API-anrop till konsol och `service.log`.