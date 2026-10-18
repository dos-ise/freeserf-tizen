# Freeserf (WebAssembly Edition)

[Freeserf](https://github.com/freeserf/freeserf) is an open-source recreation of
**The Settlers 1 (Serf City)**. This repository is a **WebAssembly port** that
makes the game playable directly in the browser – no installation required.

The code is based on the current Freeserf master (0.5, including AI opponents,
tutorials, game end, extended options, autosave, translations and TrueType
font), **deliberately on SDL2 instead of SDL3** – see below.

---

## 🎮 Play

### Prerequisite: Game data

Freeserf needs the **original game data** from The Settlers 1 (copyrighted, not
shipped with this repository). You have two options:

**Option A – Upload in the browser (recommended):** Open the game and use the
upload dialog to select your `SPA*.PA` files. No local setup needed.

**Option B – Place the files in `web/data/`:** Put one of the following files
into the `web/data/` folder:

| File    | Language   |
|---------|------------|
| `SPAE.PA` | English  |
| `SPAD.PA` | German   |
| `SPAF.PA` | French   |
| `SPAU.PA` | US English |

The files come from the DOS version of the original (Amiga data is supported
too). Without game data, the page shows an upload dialog instead of an error.

### Start

1. Serve the `web/` folder with an HTTP server (Wasm does not load via
   `file://`):
   ```
   python -m http.server 8000 --directory web
   ```
2. Open **http://localhost:8000** in your browser.
3. The game loads the data automatically and starts.

### Controls

* `1`–`5`: Activate panel buttons
* `b`: Show build options
* `TAB`/SHIFT-`TAB`: Cycle through messages
* `+`/`-`: Game speed, `0`: reset
* `p`: Pause, `j`: Switch player
* `F10`: Main menu
* `s`/`m`: Sound/music on/off
* `[`/`]`: Zoom
* CTRL+`f`: Fullscreen (also via the button)
* CTRL+`z`: Save

---

## 🔨 Build

### Locally (without CMake) – `build-wasm.ps1`

The script compiles all sources directly with `em++` (Emscripten must be
installed, e.g. via the [emsdk](https://github.com/emscripten-core/emsdk)):

```
powershell -ExecutionPolicy Bypass -File build-wasm.ps1
```

The output goes to `build-wasm/` (`FreeSerf.js` + `FreeSerf.wasm`). To test in
the browser, copy it to `web/`:

```
copy build-wasm\FreeSerf.js web\
copy build-wasm\FreeSerf.wasm web\
```

### With CMake

```
emcmake cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=OFF
cmake --build build
```

### With Docker

The `Dockerfile` builds the **Tizen widget** (`.wgt`) for Samsung Smart TVs, not
the plain WASM build. See [Tizen TV](#tizen-tv) below.

---

## 📺 Tizen TV

Freeserf runs as a **Tizen widget** (`.wgt`) on Samsung Smart TVs (Tizen 5.5+).
The app always runs **fullscreen** – only the game window, no frame or menus –
and supports the TV's **pointer device** (mouse) as well as the remote control.

### Game data from USB stick

Like GB-EMU_Tizen loads ROMs from a USB stick, the Tizen app scans every
mounted USB storage for the original Settlers 1 data files:

| File    | Language   |
|---------|------------|
| `SPAE.PA` | English  |
| `SPAD.PA` | German   |
| `SPAF.PA` | French   |
| `SPAU.PA` | US English |

Plug a USB stick with these files into the TV and start the app – it finds and
loads them automatically. If no data is found, the app shows a hint screen.

### Build the WGT

**Windows:**

```
build.bat
```

This builds the Docker image (Tizen Studio CLI + Emscripten + Freeserf compiled
to WebAssembly), assembles the Tizen widget (`config.xml`, `icon.png`,
`index.html`, `FreeSerf.js/.wasm`, `resources/`), signs and packages it, and
extracts `FreeSerf.wgt` to the current directory.

**Manually:**

```
docker build -t freeserf-tizen .
docker create --name freeserf-tmp freeserf-tizen
docker cp freeserf-tmp:/home/freeserf/FreeSerf.wgt .
docker rm freeserf-tmp
```

> **Apple Silicon note:** use `--platform linux/amd64` – Tizen Studio only ships
> x86-64 binaries.

### Build in GitHub Actions

The [`tizen.yml`](.github/workflows/tizen.yml) workflow (manual dispatch) builds
the WGT in the action and creates a GitHub release with `FreeSerf.wgt`
attached. Enter a version (e.g. `1.2`) for a proper release, or leave it empty
for a timestamped pre-release.

### Install on the TV

1. Enable **Developer Mode** on the TV (Apps screen → press `1 2 3 4 5` → toggle
   Developer Mode = ON → enter your PC's LAN IP → reboot).
2. Install the `.wgt` with the
   [Samsung Jellyfin Installer](https://github.com/Jellyfin2Samsung/Samsung-Jellyfin-Installer)
   or [Apps2Samsung](https://github.com/Apps2Samsung/Apps2Samsung/releases/latest).
3. Launch **FreeSerf** from the TV's app menu.

### Remote control mapping

| Remote key        | In-game action        |
|-------------------|-----------------------|
| D-pad arrows      | Scroll map            |
| OK / Enter        | Confirm               |
| Back / Return     | Cancel / back         |
| Red / Green / Yellow / Blue | F1 / F2 / F3 / F4 |
| Channel up / down | Page up / down (scroll) |
| Play / Pause      | Pause (space)         |
| Rewind / Fast-forward | F5 / F6            |

The pointer device (mouse) works directly on the game canvas via SDL2.

---

## 🌐 GitHub Pages

The game is hosted on GitHub Pages (see the repository settings). The
`web/` folder is deployed automatically by the
[`pages.yml`](.github/workflows/pages.yml) workflow on every push to `master`.
The pre-built `FreeSerf.js`/`FreeSerf.wasm` are committed so the site works
without a build step. Since the game data is copyrighted, the hosted version
relies on the in-browser upload dialog.

---

## ❓ Why SDL2 and not SDL3?

The Freeserf master switched to **SDL3** in October 2026. This repository stays
deliberately on **SDL2** for the WebAssembly port, because:

* **Emscripten offers no SDL3_mixer and SDL3_image ports** – SDL2_mixer,
  SDL2_image and SDL2_ttf, on the other hand, are fully available as mature
  Emscripten ports.
* SDL2 is stable and well-tested in the browser (WebGL/Canvas, WebAudio).
* The complete game logic (AI, tutorials, missions, UI, data) is
  platform-independent and comes unchanged from the current master.

The platform layer (`video-sdl`, `audio-sdlmixer`, `event_loop-sdl`,
`font-sdl`, `sprite-file-sdl`) was ported back from SDL3 to SDL2 for this.

---

## 📁 Project structure

* `web/` – the playable web app (index.html + build output + game data)
* `tizen/` – fullscreen Tizen wrapper (`index.html` with USB data loading and
  remote control support)
* `res/` – Tizen widget config (`config.xml`) and app icon
* `src/` – the Freeserf source code (upstream + SDL2 platform)
* `build-wasm.ps1` – build script without CMake
* `Dockerfile` – Tizen widget build container (Tizen Studio CLI + Emscripten)
* `build.bat` – local Tizen WGT build script
* `resources/` – the game's TrueType font and UI images

## 📄 License

Freeserf is licensed under GPL-3.0 (see `COPYING`). The original game data is
subject to the copyright of the respective rights holder.