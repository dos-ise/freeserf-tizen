# build-wasm.ps1 - Direct em++ build for freeserf-wasm (no CMake required)
# Produces build-wasm/FreeSerf.js + FreeSerf.wasm with the same flags as the
# Emscripten section in CMakeLists.txt.
#
# Usage:  powershell -ExecutionPolicy Bypass -File build-wasm.ps1
#         (or:  ./build-wasm.ps1)

$ErrorActionPreference = "Stop"

$repo = $PSScriptRoot
$src = Join-Path $repo "src"
$outDir = Join-Path $repo "build-wasm"
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

# Generate src/version-vcs.h (normally done by CMake configure_file)
$vcsTag = git -C $repo describe --always --tags --dirty 2>$null
if (-not $vcsTag) { $vcsTag = "0.3.0" }
$vcsDir = Join-Path $outDir "src"
New-Item -ItemType Directory -Force -Path $vcsDir | Out-Null
$vcsHeader = @"
#ifndef VERSION_VCS_H
#define VERSION_VCS_H

#define VERSION_VCS "$vcsTag"

#endif /* VERSION_VCS_H */
"@
Set-Content -Path (Join-Path $vcsDir "version-vcs.h") -Value $vcsHeader -Encoding ASCII
Write-Host "Generated version-vcs.h (VERSION_VCS=$vcsTag)"

# Activate Emscripten SDK
$emsdkEnv = Join-Path $env:USERPROFILE "emsdk\emsdk_env.ps1"
if (Test-Path $emsdkEnv) {
    & $emsdkEnv | Out-Null
} else {
    Write-Host "emsdk_env.ps1 not found at $emsdkEnv - assuming emcc is on PATH"
}

$sources = @(
    "debug.cc","log.cc","configfile.cc","buffer.cc",
    "ai.cc","ai-core.cc","ai-manage.cc","ai-rate.cc","ai-road.cc","ai-want.cc",
    "building.cc","flag.cc","game.cc","integrity.cc","inventory.cc","map.cc","map-generator.cc",
    "mission.cc","player.cc","random.cc","savegame.cc","serf.cc","game-manager.cc",
    "video.cc","audio.cc","event_loop.cc","video-sdl.cc","event_loop-sdl.cc","audio-sdlmixer.cc",
    "data.cc","data-source-mixed.cc","data-source-dos.cc","data-source-amiga.cc","data-source-legacy.cc",
    "data-source-custom.cc","tpwm.cc","sfx2wav.cc","xmi2mid.cc","pcm2wav.cc",
    "data-source.cc","sprite-file-sdl.cc","font-sdl.cc","translation-dummy.cc",
    "pathfinder.cc","advanced-box.cc","box-frame.cc","settings.cc","gfx.cc","viewport.cc",
    "minimap.cc","interface.cc","gui.cc","popup.cc","game-init.cc","notification.cc",
    "panel.cc","version.cc","text-input.cc","list.cc","command_line.cc","freeserf.cc"
)

$srcArgs = $sources | ForEach-Object { Join-Path $src $_ }

$compileFlags = @(
    "-std=c++11",
    "-O3", "-flto",
    "-fexceptions",
    "-sUSE_SDL=2", "-sUSE_SDL_MIXER=2", "-sUSE_SDL_IMAGE=2", "-sUSE_SDL_TTF=2",
    "-sSDL2_MIXER_FORMATS=mod,mid",
    '-DPACKAGE_BUGREPORT=\"https://github.com/freeserf/freeserf/issues\"',
    "-I$repo",
    "-I$outDir"
)

$linkFlags = @(
    "-sMODULARIZE=1", "-sEXPORT_NAME=FreeSerf",
    "-sALLOW_MEMORY_GROWTH=1",
    "-sINITIAL_MEMORY=67108864",
    "-sMAXIMUM_MEMORY=2147483648",
    "-sENVIRONMENT=web",
    "-sEXPORTED_RUNTIME_METHODS=ccall,cwrap,FS,callMain",
    "-lidbfs.js"
)

$output = Join-Path $outDir "FreeSerf.js"

Write-Host "Compiling $($sources.Count) sources with em++..."
& em++ @srcArgs @compileFlags @linkFlags -o $output

if ($LASTEXITCODE -ne 0) {
    Write-Host "BUILD FAILED (exit $LASTEXITCODE)"
    exit $LASTEXITCODE
}

Write-Host ""
Write-Host "Build OK:"
Get-ChildItem $outDir | Select-Object Name, Length
