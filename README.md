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
4. În bucla principală: răspunde la cererile de pe telefon, aplică puterea becului (calculată de PID în
   timpul unui ciclu, altfel cea aleasă în Debug), actualizează ora pe display, iar o dată pe secundă citește
   temperatura, rulează un pas al regulatorului PID și afișează temperatura pe ecran și în pagină.

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

> Ținta din Debug doar se memorează și se afișează; reglarea automată se face din modul **Normal**.
> Cât timp rulează un ciclu, becul e comandat de PID, iar puterea aleasă în Debug nu are efect.

### Normal – interfața de lucru (regulatorul PID de pe placă)

Pornește ciclul **încălzire → menținere → răcire**. Regulatorul PID și profilul rulează pe placă
(`PidDriver`); pagina setează parametrii, pornește/oprește ciclul și afișează ce raportează placa (la ~1 s).

- **Profilul de temperatură (trapez)**. La Start se reține temperatura de start `T0`, apoi:
  - **INCALZIRE**: setpoint-ul crește liniar de la `T0` la `TSET` în `tINC` secunde;
  - **MENTINERE**: setpoint-ul rămâne `TSET` timp de `tMEN` secunde;
  - **RACIRE**: setpoint-ul scade liniar de la `TSET` la `T0` în `tRAC` secunde;
  - la final PWM-ul devine 0 și ciclul se termină (`GATA`). O etapă cu durata 0 este sărită.
- **Regulatorul PID** (o dată pe secundă, ieșirea = PWM-ul becului 0–100 %):
  `eroare = setpoint − temperatura`, `I += Ki·eroare·dt`, `D = −(temperatura − temperatura_anterioară)/dt`,
  `PWM = Kp·eroare + I + Kd·D`. Cu îmbunătățirile din „Improving the Beginner's PID”: timp de eșantionare fix
  (1 s), derivata pe măsurătoare (fără salt când setpoint-ul se schimbă), Kp/Ki/Kd modificabili din mers fără
  salturi, ieșirea limitată la 0–100 % și anti-windup (termenul I limitat la 0–100). Becul nu poate răci, deci
  la răcire PWM-ul nu scade sub 0.
- **Protecție**: dacă senzorul nu răspunde în timpul ciclului, becul se stinge și ciclul se oprește
  (pagina afișează „Sensor failure”, LCD-ul `EROARE`).
- **Start/Stop** cu butonul din pagină sau din meniul LCD. Pagina afișează faza, setpoint-ul curent,
  temperatura, PWM-ul, timpul scurs și timpul rămas din etapă. Pe display-ul plăcii, la „Expected”
  apare setpoint-ul curent cât rulează ciclul.
- **LCD 16×2** desenat în pagină, cu 4 butoane: **Cancel**, **−**, **+**, **OK**
  (și de la tastatură: Enter = OK, Esc = Cancel, ↑/↓ sau +/− = modificare; ținut apăsat, `+`/`−` se repetă).
  În timpul rulării arată setpoint-ul, temperatura curentă și timpul rămas din etapă:
  ```
  S:50.0° C:49.6°
  MENTINERE  00:15
  ```
  Când nu rulează, afișează `OPRIT`, `GATA` sau `EROARE`.
- **Meniul** (OK pentru intrare, `+`/`−` pentru navigare): `Start ciclu` / `Stop ciclu`, `TSET`,
  `t incalzire`, `t mentinere`, `t racire`, `KP`, `KI`, `KD`.
  OK intră în editare (valoarea clipește), `+`/`−` o modifică, OK trimite valoarea la placă („Salvat”),
  Cancel renunță („Anulat”).
- **Parametri editabili din căsuțe text**: scrii valoarea și apeși Enter (sau atingi în afara căsuței).
  Se acceptă virgulă sau punct (`38,5`); timpii sunt în secunde. Valorile din afara intervalului sunt limitate
  automat (și pe placă). **TSET și timpii sunt blocați cât rulează un ciclu**; KP, KI, KD se pot schimba oricând.

  | Parametru | Interval | Pas | Implicit |
  |---|---|---|---|
  | TSET | 20 – 50 °C | 0,5 | 50,0 |
  | t incalzire / t mentinere / t racire | 0 – 5999 s | 1 s | 60 / 20 / 50 |
  | KP | 0 – 99,9 | 0,1 | 8,0 |
  | KI | 0 – 9,99 | 0,01 | 0,05 |
  | KD | 0 – 99,9 | 0,1 | 20,0 |

- Parametrii sunt păstrați în RAM-ul plăcii: rămân la reîncărcarea paginii, dar revin la valorile implicite
  după un reset al plăcii. „Reset to defaults” trimite valorile implicite.
- **Graficul temperaturii**: temperatura și setpoint-ul în timp, cu granițele etapelor marcate. Treci cu
  mouse-ul sau degetul peste grafic ca să vezi valorile exacte. Istoricul e ținut în browser (se pierde la
  reîncărcarea paginii); datele pot fi descărcate ca CSV.

### Testare fără placă

Dacă deschizi `Web APP/index.html` direct de pe disc (sau adaugi `?mock=1` la adresă), ambele moduri
rulează cu o placă simulată (inclusiv ciclul PID din modul **Normal**). Butonul **MOCK** din colțul modului
Debug trece prin defecte simulate (senzor defect, supratemperatură, fără conexiune).

### API HTTP

| Cerere | Efect | Răspuns |
|---|---|---|
| `GET /` | – | pagina web |
| `GET /status` | – | `{"temp":23.4,"target":22.0,"power":0,"fault":null}` |
| `GET /set?target=23.5` | setează ținta (limitată la 20–50 °C, pași de 0,5) | același JSON ca `/status` |
| `GET /pwm?duty=50` | setează puterea becului; se acceptă doar 0, 25, 50, 100 | același JSON ca `/status` |
| `GET /run` | – | `{"phase":"men","sp":50.0,"temp":49.6,"power":42,"elapsed":75,"tset":50.0,"tinc":60,"tmen":20,"trac":50,"kp":8.0,"ki":0.05,"kd":20.0}` |
| `GET /params?tset=50&tinc=60&tmen=20&trac=50&kp=8&ki=0.05&kd=20` | setează parametrii ciclului (oricare dintre ei); TSET și timpii sunt ignorați cât rulează un ciclu | același JSON ca `/run` |
| `GET /start` | pornește ciclul și pune puterea din Debug pe 0 | același JSON ca `/run` |
| `GET /stop` | oprește ciclul (PWM 0) | același JSON ca `/run` |
| orice altceva | – | `404 Not Found` |

`/status`, `/set` și `/pwm` sunt folosite de modul Debug, iar `/run`, `/params`, `/start`, `/stop` de modul Normal.
`temp` este `null` dacă senzorul nu răspunde. `fault` este deocamdată mereu `null`.
`phase` este `idle`, `inc`, `men`, `rac`, `done` sau `err` (senzor defect); `sp` este `null` când nu rulează un ciclu.
Placa aplică `/start` și `/stop` imediat după ce răspunde, așa că starea nouă apare la următorul `/run`.

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
        ├── WebDriver/               server HTTP pe portul 80: /, /status, /set, /pwm, /run, /params, /start, /stop
        │   └── WebPage.h            GENERAT din Web APP/index.html
        ├── PidDriver/               regulatorul PID și profilul încălzire / menținere / răcire
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

- Salvarea parametrilor în memoria nevolatilă a plăcii (EEPROM).
- Acordarea valorilor Kp, Ki, Kd pe montajul real.
- Protecție la supratemperatură (la senzor defect în timpul ciclului becul se stinge deja).
