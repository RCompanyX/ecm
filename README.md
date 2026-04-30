<h1 align="center">ECM-R</h1>
<p align="center"><b>Fork Notice:</b> ECM-R is a fork of the original ECM (External Custom Music) project by BttrDrgn. Keep the original license notice and review the upstream project for attribution, licensing, and development history.</p>
<h2 align="center">External Custom Music Reloaded</h2>
<p align="center">A mod for Need for Speed: Underground 2 that plays custom music without overwriting the game's original files.</p>

## Overview

ECM-R replaces or mutes the in-game music and plays audio files from a user playlist folder.
It also includes an in-game overlay for basic playback control.

This fork is currently focused on **Need for Speed: Underground 2 (NFSU2)**.

## Quick Start

1. Install ECM-R with your preferred mod loader or mod manager.
2. Download the official `bass.dll` runtime from https://www.un4seen.com/.
3. Place `ecm-r.x86.asi`, `ecm-r.x86.ini` (or let ECM-R create it), and `bass.dll` inside the target `scripts` folder.
4. Create a `Music` folder next to those runtime files.
5. Put your supported audio files inside `Music`.
6. Launch the game and open the overlay with `F11`.

ECM-R supports `frontend_volume` and `ingame_volume` in addition to the legacy `volume` setting. Older configurations keep using `volume` as the fallback base when the context-specific values are missing.

## Quick Links

- [Building](BUILDING.md)
- [Configuration Manual](CONFIGURATION.MD)
- [Changelog](CHANGELOG.md)

## Fork Status

This repository is maintained as a fork of the original ECM project by **BttrDrgn**.
The current feature set and documentation are maintained around the NFSU2 runtime used by this fork.

If you are looking for the original ECM source, attribution chain, or upstream history, review the upstream repository and the included license file.

## Requirements

- Windows
- Need for Speed: Underground 2
- A working ASI loading setup or a compatible mod manager
- Microsoft Visual C++ Redistributable (x86): https://aka.ms/vs/17/release/vc_redist.x86.exe

## Building

Build instructions for generating the plugin are available in [BUILDING.md](BUILDING.md).

## Runtime Layout

Typical deployment for this fork places the runtime files together inside the game's `scripts` folder:

```text
Game Folder/
  scripts/
    ecm-r.x86.asi
    ecm-r.x86.ini
    bass.dll
    Music/
      Artist - Song 01.mp3
      Artist - Song 02.ogg
```

ECM-R loads the official BASS runtime dynamically, so `bass.dll` must be present next to the ECM-R runtime files.

## Installation

Before choosing an installation method:

1. Download the official `bass.dll` runtime from https://www.un4seen.com/
2. Prepare the ECM-R runtime files for your target `scripts` folder:
   - `ecm-r.x86.asi`
   - `ecm-r.x86.ini` (optional on first launch)
   - `bass.dll`
3. Create a `Music` folder next to those files and place your songs inside it.

### Option 1: Mr. Modman

If you use [Mr. Modman](https://github.com/VelocityCL/mr.modman):

1. Extract the release `scripts` contents into your game's global directory or pack directory.
2. Make sure the final deployed `scripts` folder contains `ecm-r.x86.asi`, `ecm-r.x86.ini` (or allow it to be created), and `bass.dll`.
3. Make sure the `Music` folder is placed in that same runtime location.

### Option 2: ASI Loader

If your game already uses an ASI loader such as `dinput8.dll`:

1. Extract the release files into the game directory used by your loader.
2. Make sure the loader's `scripts` folder contains `ecm-r.x86.asi`, `ecm-r.x86.ini` (or allow it to be created), and `bass.dll`.
3. Create a `Music` folder in that same runtime location and place your songs there.

## Supported Audio Formats

ECM-R currently scans the playlist folder for these file types:

- `.wav`
- `.mp1`
- `.mp2`
- `.mp3`
- `.ogg`
- `.aif`

## Implemented Functionality

### Playback

- Loads custom music from a configurable playlist folder without replacing the original game files
- Supports shuffle and repeat playback modes with persistent configuration
- Supports previous and next track navigation from both hotkeys and the overlay
- Supports separate frontend and in-game volume levels, with legacy `volume` compatibility for older configurations
- Plays valid tracks in discovered order when shuffle is disabled
- Can stop playback after the last valid track when repeat is disabled
- Supports per-track routing for frontend-only, in-game-only, or shared playback

### Overlay and Controls

- Displays an in-game overlay with playback controls and playlist browsing
- Allows runtime toggles for shuffle and repeat directly from the overlay
- Includes both Previous and Next track controls in the overlay and supports configurable hotkeys for opening the overlay and changing tracks
- Uses real playback history for previous-track navigation while shuffle is enabled
- Updates the overlay chyron from filenames when track metadata follows the expected naming format

### Configuration and Persistence

- Creates `ecm-r.x86.ini` automatically on first launch
- Saves runtime changes for shuffle, repeat, and volume settings back to the configuration file
- Migrates older configurations by using legacy `volume` as the fallback source for context-specific volume settings
- Supports configurable playlist location, key bindings, loading-screen music handling, and per-track routing rules

### Game Integration

- Can stop custom music during loading screens and resume normal playback flow afterward through the `stop_music_on_loading_screens` setting, which defaults to `true`
- Allows custom music to continue through loading screens when `stop_music_on_loading_screens` is set to `false`
- Keeps the original game files untouched while replacing or muting game music through the mod runtime

## Configuration

The full configuration reference is available in [CONFIGURATION.MD](CONFIGURATION.MD).

Key supported settings include `frontend_volume`, `ingame_volume`, `shuffle_enabled`, `repeat_enabled`, `stop_music_on_loading_screens`, and `previous_track`.

## Roadmap

The current roadmap includes:

- **Pause/Resume Control** - Persistent pause state that maintains playback position
- **Multiple Playlists** - Switch between different music folders dynamically within the game
- **Advanced Context Filters** - More granular playback rules beyond FE/IG (events, game modes, etc.)
- **Volume Normalization** - Automatic level equalization across all tracks
- **Lip-Sync Synchronization** - Adjust audio synchronization for cutscenes and cinematics

## Notes

- If `bass.dll` is missing or the wrong version is loaded, audio playback will fail.
- `bass.dll` must be the official BASS runtime placed next to the ECM-R runtime files.
- The current runtime integration has been tested with BASS `v2.4.18.11`.
- Avoid using unofficial, modified, or repackaged `bass.dll` builds.
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
