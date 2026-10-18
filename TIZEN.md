# FreeSerf for Samsung Tizen TVs

FreeSerf (an open-source recreation of **The Settlers 1 / Serf City**) compiled to
WebAssembly and packaged as a **Tizen widget (`.wgt`)** for Samsung Smart TVs
(Tizen 5.5+).

The Tizen variant runs **always fullscreen** – only the game window, no frame,
no menus. It supports the TV **remote control** and the **pointer device
(mouse)**, and loads the original game data from a **USB stick** plugged into
the TV.

---

## Table of contents

- [How it works](#how-it-works)
- [Repository layout](#repository-layout)
- [Build the WGT](#build-the-wgt)
  - [Locally with Docker](#locally-with-docker)
  - [In GitHub Actions](#in-github-actions)
  - [Release workflow](#release-workflow)
- [Install on the TV](#install-on-the-tv)
- [Game data from USB stick](#game-data-from-usb-stick)
- [Controls](#controls)
  - [Remote control](#remote-control)
  - [Pointer device (mouse)](#pointer-device-mouse)
- [Troubleshooting](#troubleshooting)
- [Community package](#community-package)

---

## How it works

The game is compiled to WebAssembly with **Emscripten** (SDL2 ports) and wrapped
in a minimal Tizen web app:

```
┌─────────────────────────────────────────────────────┐
│  Tizen widget (.wgt)                                │
│  ┌───────────────────────────────────────────────┐  │
│  │ config.xml   – fullscreen, privileges,        │  │
│  │                pointing-device-support        │  │
│  ├───────────────────────────────────────────────┤  │
│  │ index.html   – fullscreen wrapper, USB data   │  │
│  │                loading, remote control, mouse │  │
│  ├───────────────────────────────────────────────┤  │
│  │ FreeSerf.js  – Emscripten runtime (lowered    │  │
│  │                for Chromium 69)               │  │
│  ├───────────────────────────────────────────────┤  │
│  │ FreeSerf.wasm– game compiled to WebAssembly   │  │
│  │                (lowered for Chromium 69)      │  │
│  ├───────────────────────────────────────────────┤  │
│  │ resources/   – settlers.ttf, button-advanced  │  │
│  └───────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────┘
```

### Tizen 5.5 (Chromium 69) compatibility

Tizen 5.5 (2020 TV sets) runs Chromium 69, whose WebAssembly has **no bulk
memory**, **no non-trapping float-to-int** and **no BigInt** at the JS/WASM
boundary. The build therefore:

1. Compiles with `-mno-bulk-memory -mno-bulk-memory-opt -mno-nontrapping-fptoint`
   and `-sWASM_BIGINT=0`.
2. Lowers the generated JS with **esbuild** (`--target=chrome69`) – Emscripten
   emits optional chaining / nullish coalescing that Chromium 69 cannot parse.
3. Lowers the WASM with **wasm-opt** to MVP features (keeping sign-ext, mutable
   globals and the atomic ops emitted by the SDL2 ports).

### Memory

The Tizen build uses **256 MB initial WASM memory** (`FREESERF_WASM_INITIAL_MEMORY`).
This avoids WASM memory growth after the WebGL textures are created – growth
invalidates the textures and caused strong black flickering on the TV.

---

## Repository layout

| Path | Purpose |
|------|---------|
| `tizen/index.html` | Fullscreen Tizen wrapper: canvas scaling, USB data loading, remote control, mouse |
| `res/config.xml` | Tizen widget manifest (`viewmodes="fullscreen"`, `pointing-device-support`, privileges) |
| `Dockerfile` | Builds the WGT: Tizen Studio CLI + Emscripten + Chromium 69 lowering + signing |
| `.github/workflows/tizen.yml` | Builds the WGT in GitHub Actions; creates releases on `workflow_dispatch` |
| `build.bat` | Local WGT build script (Windows) |
| `.dockerignore` | Keeps the Docker build context small |
| `cmake/icon.png` | App icon used in the widget |
| `CMakeLists.txt` | `FREESERF_WASM_INITIAL_MEMORY` cache variable for the WASM build |

---

## Build the WGT

### Locally with Docker

**Windows:**

```
build.bat
```

This builds the Docker image (Tizen Studio CLI 6.1 + Emscripten 6.0.10 +
FreeSerf compiled to WebAssembly), assembles the Tizen widget, signs and
packages it, and extracts `FreeSerf.wgt` to the current directory.

**Manually:**

```
docker build -t freeserf-tizen .
docker create --name freeserf-tmp freeserf-tizen
docker cp freeserf-tmp:/home/freeserf/FreeSerf.wgt .
docker rm freeserf-tmp
```

> **Apple Silicon note:** use `--platform linux/amd64` – Tizen Studio only ships
> x86-64 binaries.

### In GitHub Actions

The [`tizen.yml`](.github/workflows/tizen.yml) workflow:

- **On every push to `master`** – builds the WGT and uploads it as a run
  artifact (continuous validation).
- **On `workflow_dispatch`** – builds the WGT and creates a GitHub release with
  `FreeSerf.wgt` attached. Enter a version (e.g. `1.2`) for a proper release,
  or leave it empty for a timestamped pre-release.

### Release workflow

1. Push to `master` (or merge a PR) – the WGT is built and uploaded as an
   artifact.
2. To publish a release, run the **Release FreeSerf Tizen** workflow manually
   (Actions → workflow → Run workflow) and enter a version.
3. The release is created with `FreeSerf.wgt` attached and auto-generated
   release notes.

---

## Install on the TV

1. Enable **Developer Mode** on the TV:
   - From the Apps screen, press `1 2 3 4 5` on the remote.
   - Toggle **Developer Mode = ON**.
   - Enter your PC's LAN IP in **Host PC IP**.
   - Reboot the TV.
2. Install the `.wgt` with the
   [Samsung Jellyfin Installer](https://github.com/Jellyfin2Samsung/Samsung-Jellyfin-Installer)
   or [Apps2Samsung](https://github.com/Apps2Samsung/Apps2Samsung/releases/latest).
3. Launch **FreeSerf** from the TV's app menu.

---

## Game data from USB stick

FreeSerf needs the **original Settlers 1 data files** (copyrighted, not bundled
with the widget). The app scans every mounted USB storage for them.

**Put the files directly in the root directory of the USB stick** (FAT32 or
exFAT), not in subfolders:

```
USB stick
│
├── SPAE.PA   ← English
├── SPAD.PA   ← German
├── SPAF.PA   ← French
└── SPAU.PA   ← US English
```

- The scan looks for files matching `SPA*.PA` (case-insensitive) in the root of
  every mounted USB storage.
- At least one file is enough to play.
- The files are read in 64 KB chunks and written into the game's virtual
  filesystem (`/data/`).
- If no data is found, the app shows a hint screen.

---

## Controls

### Remote control

| Remote key | In-game action |
|------------|----------------|
| D-pad arrows | Scroll map |
| OK / Enter | Confirm |
| Back / Return | Cancel / back |
| Red / Green / Yellow / Blue | F1 / F2 / F3 / F4 |
| Channel up / down | Page up / down (scroll) |
| Play / Pause | Pause (space) |
| Rewind / Fast-forward | F5 / F6 |

The key codes are resolved at runtime via `tizen.tvinputdevice.getKey()` because
the numeric key codes differ between TV models/firmware.

### Pointer device (mouse)

The TV's pointer device (mouse mode of the remote) works directly on the game
canvas via SDL2's Emscripten backend. Enabled by
`pointing-device-support="enable"` in `res/config.xml`.

---

## Troubleshooting

| Symptom | Cause / fix |
|---------|-------------|
| Strong black flickering after start | WASM memory growth invalidated the WebGL textures. Fixed by 256 MB initial memory (`FREESERF_WASM_INITIAL_MEMORY`). |
| Game offset to the bottom-right, cut off | Canvas scaling bug: `top/left: 50%` without `translate(-50%,-50%)`. Fixed in `fitCanvasToScreen()`. |
| Loading hangs | A hung Tizen filesystem call blocked the app. All USB calls are wrapped in timeouts (5–30 s). |
| Long loading with a black screen | The loading overlay now stays visible during game initialization (map generation) and hides after 8 s. |
| WASM/JS fails to load on the TV | The build lowers JS (esbuild `chrome69`) and WASM (wasm-opt) for Chromium 69. |

---

## Community package

FreeSerf is submitted to the
[Apps2Samsung Tizen Community Packages](https://github.com/Apps2Samsung/tizen-community-packages)
bundle (issue
[#93](https://github.com/Apps2Samsung/tizen-community-packages/issues/93)).
The package manifest is:

```json
{
  "name": "FreeSerf",
  "description": "An open-source recreation of The Settlers 1 (Serf City) compiled to WebAssembly for Samsung Tizen TVs.",
  "category": "games",
  "repo": "dos-ise/freeserf-tizen",
  "source": "release",
  "branch": "release",
  "output_name": "FreeSerf.wgt"
}
```

The sync workflow pulls the `FreeSerf.wgt` asset from the latest GitHub release
of this repository.
