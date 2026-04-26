# BASS Runtime-Only Migration Plan

## Goal

Migrate ECM-R to use the official `bass.dll` only as an external runtime dependency.

The repository should no longer contain or require:

- `bass.h`
- `bass.lib`
- `deps/bass`
- post-build copying of `bass.dll`
- build-time linking against BASS

Instead, ECM-R should:

- load `bass.dll` with `LoadLibraryA`
- resolve required exports with `GetProcAddress`
- fail gracefully when `bass.dll` is missing or incompatible
- keep installation instructions pointing users to the official BASS website

## Current State

The codebase currently uses BASS through direct build-time integration.

### Source usage

- `src/app/stdafx.hpp` includes `<bass.h>`
- `src/app/audio/audio.cpp` uses:
  - `BASS_GetVersion`
  - `BASS_Init`
  - `BASS_ChannelIsActive`
  - `BASS_StreamFree`
  - `BASS_Start`
  - `BASS_Pause`
  - `BASS_SetConfig`
- `src/app/audio/player.cpp` uses:
  - `BASS_StreamCreateFile`
  - `BASS_ChannelPlay`

### Build usage

`lua/windows.lua` currently:

- adds BASS library search paths
- adds BASS include paths
- links against `bass`
- includes `bass.h` in project files
- copies `bass.dll` into the output directory in post-build steps

### Documentation

`README.md` already documents the intended runtime model well:

- users obtain `bass.dll` from the official BASS website
- `bass.dll` is required at runtime
- BASS remains subject to its own license terms

`BUILDING.md` still reflects the current post-build copy behavior and will need updating after the migration.

## Target Design

Create an internal runtime wrapper for BASS that isolates all interaction with `bass.dll`.

### New files

- `src/app/audio/bass_api.hpp`
- `src/app/audio/bass_api.cpp`

### Wrapper responsibilities

- load `bass.dll`
- resolve required exports
- expose a small internal API used by the rest of ECM-R
- hide Windows loader details from audio playback code
- centralize missing-DLL and incompatible-version handling

## Required BASS API Surface

Only the currently used API surface should be implemented.

### Functions

- `BASS_GetVersion`
- `BASS_Init`
- `BASS_ChannelIsActive`
- `BASS_StreamFree`
- `BASS_Start`
- `BASS_Pause`
- `BASS_SetConfig`
- `BASS_StreamCreateFile`
- `BASS_ChannelPlay`

### Constants

Replicate only the constants that are actually needed:

- `BASSVERSION`
- `BASS_ACTIVE_STOPPED`
- `BASS_SAMPLE_FLOAT`
- `BASS_STREAM_PRESCAN`
- `BASS_CONFIG_GVOL_STREAM`

### Types

Define only the minimum types needed by the wrapper, for example:

- `DWORD`
- `BOOL`
- `QWORD`
- `HSTREAM`

Use Windows-compatible types and keep them local to the wrapper where possible.

## Implementation Tasks

### Phase 1: Add the runtime wrapper

Create `bass_api.hpp/.cpp` with:

- `load()`
- `unload()`
- `is_available()`
- `get_version()`
- `init_device(HWND hwnd)`
- `start()`
- `pause()`
- `channel_is_active(handle)`
- `stream_free(handle)`
- `stream_create_file(path)`
- `channel_play(handle, restart)`
- `set_stream_volume_config(volume)`

The wrapper should:

- call `LoadLibraryA("bass.dll")`
- resolve all required exports during initialization
- return failure when any required export is missing

### Phase 2: Replace direct usage in audio code

Update `src/app/audio/audio.cpp` to use the wrapper instead of direct `BASS_*` calls.

Required replacements:

- `BASS_GetVersion()` -> wrapper call
- `BASS_Init(...)` -> wrapper call
- `BASS_ChannelIsActive(...)` -> wrapper call
- `BASS_StreamFree(...)` -> wrapper call
- `BASS_Start()` -> wrapper call
- `BASS_Pause()` -> wrapper call
- `BASS_SetConfig(...)` -> wrapper call

Also update initialization flow so the project:

- loads `bass.dll` before first use
- reports a clear message if `bass.dll` is missing
- reports a clear message if an incompatible BASS version is loaded

### Phase 3: Replace direct usage in player code

Update `src/app/audio/player.cpp` to use the wrapper instead of direct BASS calls.

Required replacements:

- `BASS_StreamCreateFile(...)` -> wrapper call
- `BASS_ChannelPlay(...)` -> wrapper call

Also remove the BASS example attribution comment block at the top of `player.cpp` if the file no longer contains borrowed example material.

### Phase 4: Remove `bass.h`

Update `src/app/stdafx.hpp` to remove:

- `#include <bass.h>`

No source file outside the wrapper should depend on BASS headers after the migration.

### Phase 5: Clean the build configuration

Update `lua/windows.lua` to remove all BASS build-time dependencies:

- remove BASS `syslibdirs`
- remove BASS include directories
- remove `links { "bass" }`
- remove project file references to `bass.h`
- remove post-build steps that copy `bass.dll`

The `.asi` generation steps should remain intact.

### Phase 6: Remove repository-managed BASS artifacts

After the project builds successfully without the BASS SDK:

- remove `deps/bass`

This should only happen after confirming there are no remaining source or build references.

### Phase 7: Update documentation

Update `BUILDING.md` so it no longer claims that `bass.dll` is produced by the build output.

Document that:

- ECM-R does not build or bundle `bass.dll`
- users must obtain `bass.dll` from the official BASS website
- `bass.dll` must be placed next to the deployed runtime files
- BASS remains subject to its own licensing terms

`README.md` is already broadly aligned, but it can be reviewed for minor wording updates after implementation.

## Validation Checklist

### Build validation

- project builds without `bass.h`
- project builds without `bass.lib`
- project builds without `deps/bass`
- generated output still includes the `.asi` file

### Runtime validation with `bass.dll` present

- `bass.dll` loads successfully
- audio initializes correctly
- playlist playback works
- pause/resume works
- volume changes work
- next-track playback still works

### Runtime validation with `bass.dll` missing

- ECM-R shows a clear error message
- ECM-R does not crash
- the failure mode is controlled and predictable

### Runtime validation with incompatible `bass.dll`

- ECM-R detects the version mismatch
- ECM-R reports the mismatch clearly
- ECM-R avoids undefined behavior

## Risks

Primary technical risks are limited to:

- incorrect manual function signatures
- incorrect constant values
- missing export resolution during startup
- incomplete replacement of direct BASS references

These risks are manageable because the currently used BASS surface is small.

## Recommended Execution Order

1. Create `bass_api.hpp/.cpp`
2. Replace direct BASS usage in `audio.cpp`
3. Replace direct BASS usage in `player.cpp`
4. Remove `<bass.h>` from `stdafx.hpp`
5. confirm no direct `BASS_*` references remain in project code
6. clean `lua/windows.lua`
7. build and validate
8. remove `deps/bass`
9. update `BUILDING.md`

## Licensing Notes

This migration plan is intended to support a runtime-only dependency model:

- ECM-R does not build BASS
- ECM-R does not need to bundle BASS in the repository
- users obtain the official `bass.dll` themselves

This is a technical implementation plan only and not legal advice. BASS usage and any redistribution remain subject to the official BASS license terms.
