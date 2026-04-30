# ECM-R Configuration Manual

## Overview

ECM-R creates `ecm-r.x86.ini` automatically on first launch.

Default configuration:

```ini
[core]
playlist = "Music"
volume = "100"
frontend_volume = "100"
ingame_volume = "100"
version = "..."

[config]
shuffle_enabled = true
repeat_enabled = true
volume_normalization_enabled = false
stop_music_on_loading_screens = true

[keys]
toggle_overlay = F11
previous_track = F9
skip_track = F10

[trax]
song1.mp3 = ALL
song2.mp3 = FE
song3.mp3 = IG
```

## `[core]` Settings

### `playlist`

- Defines the folder name used as the music library.
- Default value: `Music`.
- ECM-R looks for this folder relative to the mod location.

Expected runtime layout:

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

### `volume`

- Legacy fallback volume used when a specific context volume is unavailable.
- Range: `0` to `100`.
- Default value: `100`.

### `frontend_volume`

- Volume used while ECM-R is playing in frontend or menu contexts.
- Range: `0` to `100`.
- Default value: `100`.
- Existing configurations inherit the value from `volume` when this setting is missing.

### `ingame_volume`

- Volume used while ECM-R is playing during in-game or racing contexts.
- Range: `0` to `100`.
- Default value: `100`.
- Existing configurations inherit the value from `volume` when this setting is missing.

### `version`

- Internal version marker managed by ECM-R.
- This value is updated automatically by the mod.

## `[config]` Settings

### `shuffle_enabled`

- Controls whether valid tracks are shuffled before playback.
- Default value: `true`.
- When set to `false`, ECM-R plays valid tracks in discovered order.

### `repeat_enabled`

- Controls whether playback restarts from the beginning of the valid track list after the last song finishes.
- Default value: `true`.
- When set to `false`, playback stops after the last valid track in the current context.

### `stop_music_on_loading_screens`

- Controls whether ECM-R stops the current song during loading screens.
- Default value: `true`.
- When set to `false`, custom music continues through loading screens.

### `volume_normalization_enabled`

- Enables optional per-track volume normalization.
- Default value: `false`.
- When enabled, ECM-R prefers ReplayGain-style metadata when present and falls back to cached loudness analysis for tracks without supported metadata.
- Normalization is applied on top of the active frontend or in-game volume setting.

### Accepted boolean values

The `[config]` section accepts these values:

- `true`
- `false`
- `1`
- `0`
- `yes`
- `no`
- `on`
- `off`

## `[keys]` Settings

### `toggle_overlay`

- Key used to show or hide the in-game overlay.
- Default value: `F11`.

### `previous_track`

- Key used to go back to the previous song.
- Default value: `F9`.

### `skip_track`

- Key used to jump to the next song.
- Default value: `F10`.

### Supported key values

ECM-R accepts:

- Function keys such as `F1` to `F24`
- Letters such as `A` to `Z`
- Digits such as `0` to `9`
- Named keys such as `Space`, `Tab`, `Enter`, `Esc`, `Escape`, `Backspace`, `Insert`, `Delete`, `Home`, `End`, `PageUp`, `PageDown`, `Up`, `Down`, `Left`, and `Right`

If a key value is invalid, ECM-R falls back to the built-in default for that action.

## `[trax]` Settings

Each discovered song is added under `[trax]`.

Allowed values:

- `ALL`: Play everywhere
- `FE`: Frontend only
- `IG`: In-game only

This makes it possible to separate menu music from racing music.
Invalid or missing values fall back to `ALL`.

## Behavior Notes

- Changes made from the overlay to shuffle, repeat, or volume settings are saved back to `ecm-r.x86.ini` automatically.
- Loudness analysis results are cached in `ecm-r.volume-cache.txt` next to the runtime files so tracks do not need to be re-analyzed on every launch.
- If a filename follows the format `Artist - Title.ext`, the overlay chyron uses that information when possible.
- Supported playlist formats are `.wav`, `.mp1`, `.mp2`, `.mp3`, `.ogg`, and `.aif`.
