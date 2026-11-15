# Regulator de temperatură

Regulator de temperatură construit pe un **Arduino UNO R4 WiFi**. Placa citește temperatura cu un senzor
**DS18B20**, încălzește cu un **bec** comandat prin PWM (modul MOSFET), afișează valorile pe un display
**ILI9341** și poate fi controlată de pe telefon printr-o **interfață web** servită chiar de Arduino.

- `Microcontroler/` – firmware-ul pentru Arduino UNO R4 WiFi (display, WiFi, server web, senzor, încălzitor)
- `Web APP/` – interfața web (`index.html`), servită de Arduino
- `tools/embed_page.py` – copiază `Web APP/index.html` în firmware
- `docs/SystemFlow.md` – cum funcționează sistemul: module, pornire, bucla principală, API HTTP, diagrame

## Cum funcționează pe scurt

```
 Telefon (browser)  <--- WiFi (hotspotul telefonului) --->  Arduino UNO R4 WiFi
                                                              ├── display ILI9341 (SPI)
                                                              ├── senzor DS18B20 (pin 2)
                                                              └── modul MOSFET + bec (pin 3, PWM)
```

1. La pornire, placa stinge becul, pornește display-ul și se conectează la hotspotul telefonului.
2. Ia ora de pe internet (NTP) și o păstrează în ceasul intern (RTC).
3. Pornește un server web pe portul 80. Adresa lui apare jos pe display: `Web: http://<ip>`.
4. În bucla principală: răspunde la cererile de pe telefon, aplică puterea becului aleasă în pagină,
   actualizează ora pe display, iar o dată pe secundă citește temperatura și o afișează pe ecran și în pagină.

Regula de arhitectură: fișierul `.ino` doar apelează funcții ale driverelor și mută datele între ele.
Driverele nu se apelează între ele (de exemplu `WebDriver` nu știe că există display-ul).

## Interfața web

Când deschizi pagina, alegi între două moduri:

### Debug – legătura reală cu placa

Aceasta este interfața care vorbește cu Arduino-ul:

- **Temperatura curentă** citită de DS18B20 (sau `--` dacă senzorul lipsește).
- **Temperatura țintă**: o reglezi cu `−` / `+` sau cu sliderul (20–50 °C, pași de 0,5 °C) și o trimiți cu **Set**.
  Valoarea apare și pe display la „Expected temperature”.
- **Puterea becului**: Off / 25% / 50% / 100%, aplicată cu **Apply**. Becul pornește treptat (≈0,5 s),
  ca să nu suprasolicite sursa când filamentul e rece.
- Pagina întreabă placa de stare la fiecare 2 secunde. Dacă 2 cereri la rând eșuează, apare mesajul
  „No connection to the regulator. Retrying…” și pagina se reconectează singură.
- Cererile nu se suprapun niciodată, pentru că placa servește un singur client pe rând.

> Momentan nu există reglare automată: ținta doar se memorează și se afișează, iar puterea becului se alege manual.

### Normal – interfața finală (doar în browser, simulată)

Aceasta este interfața cerută în specificații. **Deocamdată nu comunică cu placa**: încălzitorul, senzorul,
regulatorul PID și ciclul de lucru sunt simulate în browser, ca să poată fi testată înainte să existe în firmware.

- **LCD 16×2** desenat în pagină, cu 4 butoane: **Cancel**, **−**, **+**, **OK**
  (și de la tastatură: Enter = OK, Esc = Cancel, ↑/↓ sau +/− = modificare; ținut apăsat, `+`/`−` se repetă).
- **În timpul rulării**, LCD-ul arată temperatura setată, temperatura curentă și timpul rămas din etapa actuală:
  ```
  S:45.0° C:44.3°
  MENTINERE  06:00
  ```
  Etapele sunt **INCALZIRE** (temperatura urcă în rampă până la TSET), **MENTINERE** (rămâne la TSET) și
  **RACIRE** (coboară în rampă spre temperatura camerei). Când nu rulează, afișează `OPRIT` sau `GATA`.
- **Meniul** (OK pentru intrare, `+`/`−` pentru navigare): `Start ciclu` / `Stop ciclu`, `TSET`,
  `t incalzire`, `t mentinere`, `t racire`, `KP`, `KI`, `KD`.
  OK intră în editare (valoarea clipește), `+`/`−` o modifică, OK salvează („Salvat”), Cancel renunță („Anulat”).
- **Parametri editabili din căsuțe text**, de pe telefon sau de pe PC: scrii valoarea și apeși Enter
  (sau atingi în afara căsuței). Se acceptă virgulă sau punct (`38,5`), iar timpii se scriu `mm:ss` sau în
  minute (`5.5` = 05:30). Valorile din afara intervalului sunt limitate automat.

  | Parametru | Interval | Pas | Implicit |
  |---|---|---|---|
  | TSET | 20 – 50 °C | 0,5 | 45,0 |
  | t incalzire / t mentinere / t racire | 00:00 – 99:50 | 10 s | 05:00 / 10:00 / 05:00 |
  | KP | 0 – 99,9 | 0,1 | 8,0 |
  | KI | 0 – 9,99 | 0,01 | 0,05 |
  | KD | 0 – 99,9 | 0,1 | 20,0 |

- **Memorie nevolatilă**: parametrii se salvează în browser (în locul memoriei EEPROM a plăcii), deci
  rămân și după reîncărcarea paginii. „Reset to defaults” readuce valorile implicite.
- **Graficul temperaturii** (interfața de PC opțională): temperatura și profilul de consemn în timp,
  cu granițele etapelor marcate. Treci cu mouse-ul sau degetul peste grafic ca să vezi valorile exacte.
  Simularea poate fi accelerată (1×, 10×, 60×), iar datele pot fi descărcate ca CSV.

### Testare fără placă

Dacă deschizi `Web APP/index.html` direct de pe disc (sau adaugi `?mock=1` la adresă), modul **Debug**
rulează cu o placă simulată. Butonul **MOCK** din colț trece prin defecte simulate
(senzor defect, supratemperatură, fără conexiune). Modul **Normal** e oricum simulat.

### API HTTP (folosit de modul Debug)

| Cerere | Efect | Răspuns |
|---|---|---|
| `GET /` | – | pagina web |
| `GET /status` | – | `{"temp":23.4,"target":22.0,"power":0,"fault":null}` |
| `GET /set?target=23.5` | setează ținta (limitată la 20–50 °C, pași de 0,5) | același JSON ca `/status` |
| `GET /pwm?duty=50` | setează puterea becului; se acceptă doar 0, 25, 50, 100 | același JSON ca `/status` |
| orice altceva | – | `404 Not Found` |

`temp` este `null` dacă senzorul nu răspunde. `fault` este deocamdată mereu `null`.

## Instalarea proiectului Arduino

### 1. Librării

Toate se instalează din Arduino Library Manager. Versiunile testate:

| Librărie | Versiune |
|---|---|
| Adafruit ILI9341 | 1.6.4 |
| Adafruit GFX Library | 1.12.6 |
| Adafruit BusIO | 1.17.4 |
| Adafruit STMPE610 | 1.1.6 |
| Adafruit TouchScreen | 1.1.6 |
| Adafruit TSC2007 | 1.1.2 |
| Adafruit SH110X | 2.1.15 |
| OneWire | ultima versiune |
| DallasTemperature | ultima versiune |
| NTPClient | ultima versiune |

`WiFiS3` și `RTC` vin odată cu pachetul plăcii UNO R4, nu trebuie instalate separat.

**Varianta A – Arduino IDE (cea mai simplă):**
*Tools → Manage Libraries…*, caută **Adafruit ILI9341** și apasă *Install*. Când întreabă de dependențe,
apasă **Install All**. Apoi instalează la fel **OneWire**, **DallasTemperature** și **NTPClient**.

**Varianta B – arduino-cli (versiuni exacte):**

```sh
arduino-cli lib install "Adafruit ILI9341@1.6.4" "Adafruit GFX Library@1.12.6" "Adafruit BusIO@1.17.4" "Adafruit STMPE610@1.1.6" "Adafruit TouchScreen@1.1.6" "Adafruit TSC2007@1.1.2" "Adafruit SH110X@2.1.15" "OneWire" "DallasTemperature" "NTPClient"
```

### 2. Încărcarea pe placă

Deschide `Microcontroler/Main/sketch_oct2a/sketch_oct2a.ino` în Arduino IDE, alege placa
**Arduino UNO R4 WiFi** și portul ei, apoi apasă **Upload**.

### 3. Deschiderea paginii pe telefon

1. Pornește hotspotul **Radu's Galaxy A51** (parola `12345678`). Trebuie să fie pe banda **2.4 GHz** și cu
   securitate **WPA2** – UNO R4 nu se poate conecta la rețele de 5 GHz sau doar WPA3.
2. Resetează placa. Display-ul pornește imediat și arată jos `Connecting...` cât timp se conectează
   (până la 20 s), apoi `Web: http://<ip>`.
3. Pe telefon, deschide adresa respectivă în browser și alege **Normal** sau **Debug**.

Serial Monitor (9600 baud) afișează și el pașii conectării și adresa IP.
Numele și parola rețelei se schimbă în `src/Drivers/WifiDriver/WifiDriver.cpp`.

### Modificarea paginii web

Pagina este compilată în firmware, așa că după orice modificare în `Web APP/index.html` trebuie rulat:

```sh
python tools/embed_page.py
```

și apoi refăcut upload-ul. Scriptul regenerează `src/Drivers/WebDriver/WebPage.h`
(fișierul acesta nu se editează niciodată de mână). **Dacă pe telefon vezi încă pagina veche, acest pas lipsește.**

## Structura proiectului

```
Temperature_Regulator/
├── README.md
├── docs/SystemFlow.md               documentație detaliată cu diagrame
├── tools/embed_page.py              index.html -> WebPage.h
├── Web APP/index.html               interfața web (Normal + Debug)
└── Microcontroler/Main/sketch_oct2a/
    ├── sketch_oct2a.ino             programul principal – doar apelează drivere
    └── src/Drivers/
        ├── WifiDriver/              conectarea la hotspot, reconectare, adresa IP
        ├── WebDriver/               server HTTP pe portul 80: /, /status, /set, /pwm
        │   └── WebPage.h            GENERAT din Web APP/index.html
        ├── DispDriver/              desenarea pe display-ul ILI9341
        ├── RtcDriver/               ora din NTP, păstrată în RTC-ul intern
        ├── DigitalTempDriver/       senzorul DS18B20 (citire fără blocare)
        ├── HeaterDriver/            becul prin PWM, cu pornire treptată
        └── TempDriver/              senzorul LM35 (păstrat, dar nefolosit)
```

Fiecare driver are un fișier `.h` (funcțiile publice, cu prefixul driverului, de ex. `Wifi_Init`) și un `.cpp`
(implementarea). Driverele stau în `src/` pentru că Arduino IDE compilează doar fișierele din folderul
sketch-ului, iar tot ce e în `src/` este compilat automat.
Pentru un driver nou: creează `src/Drivers/<Nume>/<Nume>.h` și `.cpp`, apoi include-l în `.ino` cu
`#include "src/Drivers/<Nume>/<Nume>.h"`.

## Conexiuni

| Componentă | Pin Arduino |
|---|---|
| Display ILI9341 – CS | 10 |
| Display ILI9341 – DC | 9 |
| Display ILI9341 – RST | 8 |
| Display ILI9341 – MOSI / MISO / SCK | SPI hardware |
| DS18B20 – DQ (modulul are rezistența de 4,7 kΩ) | 2 |
| Modul MOSFET – SIG (comandă becul) | 3 |
| LM35 (nefolosit momentan) | A1 |

Becul este alimentat de la o sursă separată, prin modulul MOSFET; masa sursei trebuie legată la GND-ul plăcii.

## Ce urmează

- Reglarea automată (PID) a temperaturii pe placă, cu etapele încălzire / menținere / răcire.
- Salvarea parametrilor în memoria nevolatilă a plăcii și legarea modului **Normal** de firmware.
- Protecții: oprirea becului la senzor defect sau la supratemperatură (pagina știe deja să le afișeze).
