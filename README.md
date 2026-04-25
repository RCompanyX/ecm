<h1 align="center">ECM-R</h1>
<p align="center"><b>Fork Notice:</b> ECM-R is a fork of the original ECM (External Custom Music) project by BttrDrgn. Keep the original license notice and review the upstream project for attribution, licensing, and development history.</p>
<h2 align="center">External Custom Music Reloaded</h2>
<p align="center">A mod for Need for Speed: Underground 2 that plays custom music without overwriting the game's original files.</p>

## Overview

ECM-R replaces or mutes the in-game music and plays audio files from a user playlist folder.
It also includes an in-game overlay for basic playback control.

Current behavior in this fork is focused on **NFSU2**.

## Fork Status

This repository is maintained as a fork.
If you are looking for the original ECM source, attribution chain, or upstream history, review the upstream repository and the included license file.

Original author of ECM: **BttrDrgn**.

## Requirements

- Windows
- Need for Speed: Underground 2
- A working ASI loading setup or a compatible mod manager
- Microsoft Visual C++ Redistributable (x86): https://aka.ms/vs/17/release/vc_redist.x86.exe

## Building

Build instructions for generating the plugin are available in [BUILDING.md](BUILDING.md).

## Installation

### Option 1: Mr. Modman

If you use [Mr. Modman](https://github.com/VelocityCL/mr.modman):

1. Extract the files from the `scripts` folder into your game's global directory or pack directory.
2. Make sure `ecm-r.x86.asi` is included.
3. Obtain `bass.dll` from the official BASS website if it is not bundled with your setup: https://www.un4seen.com/
3. Create a folder named `Music` next to the mod files.
4. Put your songs inside that folder.

### Option 2: ASI Loader

If your game already uses an ASI loader such as `dinput8.dll`:

1. Extract the release files into the game directory.
2. Make sure `ecm-r.x86.asi` is present.
3. Obtain `bass.dll` from the official BASS website if it is not already available in your game setup: https://www.un4seen.com/
3. Create a `Music` folder in the expected mod location.
4. Put your songs inside that folder.

## Supported Audio Formats

ECM-R currently scans the playlist folder for these file types:

- `.wav`
- `.mp1`
- `.mp2`
- `.mp3`
- `.ogg`
- `.aif`

## How It Works

- ECM-R loads music from the configured playlist folder.
- The game's own music volume is muted by the mod.
- Songs are shuffled automatically.
- When a track ends, the next one is played.
- Songs can be tagged for frontend-only or in-game-only playback through the configuration file.
- Some intro or movie screens temporarily pause playback.

If a filename follows the format `Artist - Title.ext`, the overlay chyron uses that information when possible.

## Controls

- `F11`: Toggle the in-game overlay by default
- `F10`: Skip to the next song by default

Both hotkeys can be changed in `ecm-r.x86.ini`.

## Overlay

The overlay provides:

- An **Actions** menu
  - Volume slider
  - Skip button
- A **Playlist** menu
  - Displays the discovered songs
- A status line showing the current song and playlist name

## Configuration

ECM-R creates a configuration file automatically on first launch:

- `ecm-r.x86.ini`

Default configuration:

```ini
[core]
playlist = "Music"
volume = "100"
version = "..."

[keys]
toggle_overlay = F11
skip_track = F10

[trax]
song1.mp3 = ALL
song2.mp3 = FE
song3.mp3 = IG
```

### Core Settings

- `playlist`: Folder name used as the music library. Default is `Music`.
- `volume`: Playback volume from `0` to `100`.
- `version`: Internal version marker used by the mod.

### Key Bindings

- `toggle_overlay`: Key used to show or hide the overlay.
- `skip_track`: Key used to jump to the next song.

Supported values include:

- Function keys such as `F1` to `F24`
- Letters such as `A` to `Z`
- Digits such as `0` to `9`
- Named keys such as `Space`, `Tab`, `Enter`, `Escape`, `Insert`, `Delete`, `Home`, `End`, `PageUp`, `PageDown`, `Up`, `Down`, `Left`, and `Right`

### Track Routing

Each discovered song is added under `[trax]`.

Allowed values:

- `ALL`: Play everywhere
- `FE`: Frontend only
- `IG`: In-game only

This lets you separate menu music from racing music.

## Playlist Folder

By default, ECM-R looks for a folder named `Music` relative to the mod location.

Example:

```text
Game Folder/
  scripts/
    ecm-r.x86.asi
    bass.dll
    Music/
      Artist - Song 01.mp3
      Artist - Song 02.ogg
```

## Notes

- If `bass.dll` is missing or the wrong version is loaded, audio playback will fail.
- This project uses the native BASS C/C++ API and requires `bass.dll` at runtime.
- `bass.dll` can be obtained from the official BASS website: https://www.un4seen.com/
- BASS is a third-party library and should be used or redistributed according to the applicable BASS license terms.
- ECM-R is distributed as a non-commercial fork project. If your usage, redistribution, or packaging of BASS goes beyond non-commercial use, review the official BASS licensing terms before shipping it.
- The mod writes a crash dump file on unhandled exceptions.
- This repository includes third-party dependencies and keeps the original MIT license notice.
- The fork branding is ECM-R, and the runtime filenames follow the `ecm-r.*` naming scheme.

## License

This project includes an MIT license file that retains the original copyright notice.
If you redistribute this fork, keep the existing license and attribution intact.
