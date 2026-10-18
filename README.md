# Freeserf (WebAssembly Edition)

[Freeserf](https://github.com/freeserf/freeserf) ist eine Open-Source-Nachbildung
von **The Settlers 1 (Serf City)**. Dieses Repository ist ein **WebAssembly-Port**,
der das Spiel direkt im Browser spielbar macht – ohne Installation.

Der Code basiert auf dem aktuellen Freeserf-Master (Stand 0.5, inkl. KI-Gegner,
Tutorials, Spielende, erweiterte Optionen, Autosave, Übersetzungen und
TrueType-Font), **bewusst auf SDL2 statt SDL3** – siehe unten.

---

## 🎮 Spielen

### Voraussetzung: Spieldaten

Freeserf benötigt die **Original-Spieldaten** von The Settlers 1 (urheberrechtlich
geschützt, werden hier nicht mitgeliefert). Lege eine der folgenden Dateien in
den Ordner `web/data/`:

| Datei | Sprache |
|-------|---------|
| `SPAE.PA` | Englisch |
| `SPAD.PA` | Deutsch |
| `SPAF.PA` | Französisch |
| `SPAU.PA` | US-Englisch |

Die Dateien stammen aus der DOS-Version des Originals (auch Amiga-Daten werden
unterstützt). Ohne Spieldaten zeigt die Seite eine Fehlermeldung.

### Starten

1. Den Ordner `web/` mit einem HTTP-Server ausliefern (Wasm lädt nicht per
   `file://`):
   ```
   python -m http.server 8000 --directory web
   ```
2. Im Browser **http://localhost:8000** öffnen.
3. Das Spiel lädt die Spieldaten automatisch und startet.

### Steuerung

* `1`–`5`: Panel-Buttons aktivieren
* `b`: Bau-Möglichkeiten anzeigen
* `TAB`/SHIFT-`TAB`: Nachrichten durchblättern
* `+`/`-`: Spielgeschwindigkeit, `0`: zurücksetzen
* `p`: Pause, `j`: Spieler wechseln
* `F10`: Hauptmenü
* `s`/`m`: Sound/Musik an/aus
* `[`/`]`: Zoom
* CTRL+`f`: Vollbild (auch über den Button)
* CTRL+`z`: Speichern

---

## 🔨 Bauen

### Lokal (ohne CMake) – `build-wasm.ps1`

Das Skript kompiliert alle Quellen direkt mit `em++` (Emscripten muss installiert
sein, z. B. über das [emsdk](https://github.com/emscripten-core/emsdk)):

```
powershell -ExecutionPolicy Bypass -File build-wasm.ps1
```

Die Ausgabe liegt in `build-wasm/` (`FreeSerf.js` + `FreeSerf.wasm`). Für den
Test in den Browser nach `web/` kopieren:

```
copy build-wasm\FreeSerf.js web\
copy build-wasm\FreeSerf.wasm web\
```

### Mit CMake

```
emcmake cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=OFF
cmake --build build
```

### Mit Docker

```
docker build -t freeserf-wasm .
```

---

## ❓ Warum SDL2 und nicht SDL3?

Der Freeserf-Master ist im Oktober 2026 auf **SDL3** umgestiegen. Für den
WebAssembly-Port bleibt dieses Repository bewusst auf **SDL2**, weil:

* **Emscripten bietet keine SDL3_mixer- und SDL3_image-Ports** – SDL2_mixer,
  SDL2_image und SDL2_ttf sind dagegen vollständig als Emscripten-Ports
  verfügbar und ausgereift.
* SDL2 ist im Browser (WebGL/Canvas, WebAudio) stabil und gut getestet.
* Die komplette Spiellogik (KI, Tutorials, Missionen, UI, Daten) ist
  plattformunabhängig und stammt unverändert aus dem aktuellen Master.

Die Plattform-Schicht (`video-sdl`, `audio-sdlmixer`, `event_loop-sdl`,
`font-sdl`, `sprite-file-sdl`) wurde dafür von SDL3 auf SDL2 zurückportiert.

---

## 📁 Projektstruktur

* `web/` – die spielbare Web-App (index.html + Build-Ausgabe + Spieldaten)
* `src/` – der Freeserf-Quellcode (Upstream + SDL2-Plattform)
* `build-wasm.ps1` – Build-Skript ohne CMake
* `Dockerfile` – Build-Container
* `resources/` – TrueType-Font und UI-Bilder des Spiels

## 📄 Lizenz

Freeserf ist unter der GPL-3.0 lizenziert (siehe `COPYING`). Die Spieldaten des
Originals unterliegen dem Copyright des jeweiligen Rechteinhabers.
