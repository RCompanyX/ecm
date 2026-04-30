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
3. Download `bass.dll` from the official BASS website: https://www.un4seen.com/
4. Place `bass.dll` in the same `scripts` folder as ECM-R.
5. Make sure `ecm-r.x86.ini` is present or allow ECM-R to create it on first launch.
6. Create a folder named `Music` next to the mod files.
7. Put your songs inside that folder.

### Option 2: ASI Loader

If your game already uses an ASI loader such as `dinput8.dll`:

1. Extract the release files into the game directory.
2. Make sure `ecm-r.x86.asi` is present.
3. Download `bass.dll` from the official BASS website: https://www.un4seen.com/
4. Place `bass.dll` in the same `scripts` folder as ECM-R.
5. Make sure `ecm-r.x86.ini` is present or allow ECM-R to create it on first launch.
6. Create a `Music` folder in the expected mod location.
7. Put your songs inside that folder.

## Supported Audio Formats

ECM-R currently scans the playlist folder for these file types:

- `.wav`
- `.mp1`
- `.mp2`
- `.mp3`
- `.ogg`
- `.aif`

## Implemented Functionality

- Loads custom music from a configurable playlist folder without replacing the original game files
- Displays an in-game overlay with playback controls and playlist browsing
- Supports shuffle and repeat playback modes with persistent configuration
- Supports separate frontend and in-game volume levels
- Supports per-track routing for frontend-only, in-game-only, or shared playback
- Can stop custom music during loading screens and resume normal playback flow afterward
- Allows configurable hotkeys for opening the overlay and changing tracks

## Controls

- `F11`: Toggle the in-game overlay by default
- `F9`: Go back to the previous song by default
- `F10`: Skip to the next song by default

Both hotkeys can be changed in `ecm-r.x86.ini`.

## Configuration

The full configuration reference is available in [docs/configuration-manual.md](docs/configuration-manual.md).

## Planned Features

The following features are planned for future releases:

- **Pause/Resume Control** - Persistent pause state that maintains playback position
- **Multiple Playlists** - Switch between different music folders dynamically within the game
- **Advanced Context Filters** - More granular playback rules beyond FE/IG (events, game modes, etc.)
- **Lip-Sync Synchronization** - Adjust audio synchronization for cutscenes and cinematics
- **Volume Normalization** - Automatic level equalization across all tracks
- **Real-Time Audio Format Conversion** - Support for additional audio formats through runtime conversion

## Notes

- If `bass.dll` is missing or the wrong version is loaded, audio playback will fail.
- This project loads the official BASS runtime dynamically and requires `bass.dll` at runtime.
- Download `bass.dll` from the official BASS website: https://www.un4seen.com/
- Place `bass.dll` in the same `scripts` folder as ECM-R runtime files.
- The current runtime integration has been tested with BASS `v2.4.18.11`.
- BASS is a third-party library and remains subject to the official BASS license terms.
- ECM-R does not bundle or redistribute `bass.dll`; users must obtain the official runtime themselves.
- ECM-R is maintained as a non-commercial fork project.
- If your usage of ECM-R or BASS becomes commercial, review the official BASS licensing terms and obtain any required licence before distribution.
- The mod writes a crash dump file on unhandled exceptions.
- This repository includes third-party dependencies and keeps the original MIT license notice.
- The fork branding is ECM-R, and the runtime filenames follow the `ecm-r.*` naming scheme.

## License

This project includes an MIT license file that retains the original copyright notice.
If you redistribute this fork, keep the existing license and attribution intact.
