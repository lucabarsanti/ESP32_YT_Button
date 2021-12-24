# Smart Youtube Button with Subscribers Counter ESP32

L'iconico Button di Youtube rivisitato e "smartizzato" con un contatore di iscritti in tempo reale!

![YTButton](https://raw.githubusercontent.com/lucabarsanti/ESP32_YT_Button/main/img/thumb.jpg "YTButton")


### Dipendenze
Software
* [PlatformIO](https://platformio.org/platformio-ide "PlatformIO")

Librerie
* ArduinoJson ( per decodificare i dati da Google )
* MD_Parola ( per scrivere i dati sul display )

API
* API Key di Google Cloud Platform

### Lista della spesa (con link)

* ESP32 ( [DEV](https://amzn.to/3mPKY1h "DEV") oppure [Mini](https://amzn.to/30SiSKK "Mini") )
* Display [MAX7219](https://amzn.to/3ejAodZ "MAX7219") ( io ne ho usati 6 moduli )
* [Cornice](https://amzn.to/3JhC6e4 "Cornice") con vetro
* Lastra Plexiglass 4mm
* Cavo usb
* [Logo YT](https://www.thingiverse.com/thing:3385754 "Logo YT") stampato in 3D

## Modificare il numero di moduli display

Io ne ho usati 6, se ne avete un numero diverso, modificate questa definizione:
```
#define MAX_DEVICES 6
```

## Modificare la scheda

In base alla scheda utilizzata, impostate sul file "platformio.ini" la board corretta ( potete usare ad inizio linea il "punto e virgola" per commentare le linee inutilizzate )
```
board = esp32dev
board = wemos_d1_mini32
```

## Autore

[Luca Barsanti](www.foxsource.it)

## Releases

* 0.1
    * Prima release
