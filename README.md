# 2048 for PS Vita

A homebrew build of 2048 for the PlayStation Vita, written in C with
[vita2d](https://github.com/xerpi/libvita2d).

## Controls

| Input | Action |
| --- | --- |
| D-Pad / Left stick | Slide the tiles |
| Triangle | New game |
| Start | Quit to LiveArea |

The best score is saved to `ux0:data/2048/best.bin`.

## Getting the VPK

### Option A — build in the cloud (no toolchain needed)

Push this folder to a GitHub repo. The workflow in
[.github/workflows/build.yml](.github/workflows/build.yml) builds inside the
official `vitasdk/vitasdk` container and generates the LiveArea artwork with
ImageMagick. Grab `vita2048.vpk` from the run's **Artifacts** section.

```sh
cd 2048vita
git init && git add . && git commit -m "2048 for PS Vita"
gh repo create 2048vita --private --source=. --push
```

### Option B — build locally

Requires [VitaSDK](https://vitasdk.org/) with `$VITASDK` set, plus CMake.
You also need to supply the three LiveArea PNGs yourself (see the workflow for
the exact sizes: `icon0.png` 128x128, `bg.png` 840x500, `startup.png` 280x158).

```sh
cmake -S . -B build
cmake --build build
# -> build/vita2048.vpk
```

## Installing on the Vita

1. Copy `vita2048.vpk` to the Vita (VitaShell's FTP, or USB).
2. In VitaShell, highlight the VPK and press **X** to install.
3. Launch **2048** from the LiveArea.

## Layout

- [src/board.c](src/board.c) — the 4x4 grid: slide, merge, spawn, game-over check
- [src/main.c](src/main.c) — game loop, controller input, vita2d rendering
- [CMakeLists.txt](CMakeLists.txt) — build and VPK packaging

The title ID is `NARM20481`. If you ever want two builds installed side by side,
change `VITA_TITLEID` in [CMakeLists.txt](CMakeLists.txt) — it must be 9
characters: exactly 4 uppercase letters followed by 5 digits. The installer
rejects anything else with error 0x8010113D.
