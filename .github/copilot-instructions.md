# Copilot Instructions

## General Guidelines
- Project documentation and commit messages should be written in English.
- Project documentation and written project content should be in English.
- Repository instruction files should be written in English.
- User-facing documentation, release notes, and changelog entries should be written in English.
- Keep fork attribution to the original ECM project and its original author, BttrDrgn.
- Do not remove or rewrite the original license attribution.
- Preserve compatibility-sensitive runtime filenames unless a task explicitly requires changing them.
- New work branches must start with the prefix `dev_`.

## Code Style
- Prefer a shared helper for playlist navigation with small wrappers like `play_next_song()` and `play_previous_song()`, instead of using boolean flags for direction.

## Release Documentation
- For release documentation, only include features introduced in the target release and do not mix in items from previous releases.

## Project-Specific Rules
- Migrate the project to manual dynamic loading of `bass.dll` using `LoadLibrary` and `GetProcAddress`.
- Eliminate compile-time references to BASS (`bass.h`, `bass.lib`, `deps/bass`).
- Maintain BASS only as an official runtime dependency downloaded externally.
- The expected deployment of the mod places `ecm-r` and `bass.dll` together within the `scripts` folder (e.g., `scripts/ecm-r.dll`, `scripts/ecm-r.ini`, `scripts/bass.dll`).
