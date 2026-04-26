# Changelog

All notable changes to ECM-R are documented in this file.

This changelog currently tracks the tagged releases recorded in this repository.

## [v0.5.2-alpha] - 2026-04-26

### Added
- Added configurable `shuffle_enabled` and `repeat_enabled` playlist options.
- Added overlay controls to toggle shuffle and repeat modes at runtime.

### Changed
- Playlist playback can now run in sequential mode when shuffle is disabled.
- Playlist looping can now be disabled so playback stops after the last valid track.
- The generated `ecm-r.x86.ini` file now persists shuffle and repeat settings.

### Documentation
- Updated the changelog for the new playlist playback options.
- Updated project documentation to describe the new configuration entries and overlay behavior.

## [v0.5.1-alpha] - 2026-04-25

### Added
- Added the `stop_music_on_loading_screens` configuration option.

### Changed
- Updated runtime and configuration filenames to the `ecm-r.*` naming scheme.
- Improved configuration version handling.

### Documentation
- Updated documentation for the new loading screen music behavior.
- Refreshed general project documentation.

## [v0.5.0-alpha] - 2026-04-24

### Added
- First tagged ECM-R alpha release.
- Added configurable key bindings for overlay toggle and track skipping.
- Added build documentation for generating the plugin.

### Changed
- Rebranded the fork to ECM-R.
- Improved playback behavior across game phases.
- Added persistent volume saving.
- Updated project packaging and ignored user-specific or generated files.

### Documentation
- Updated the README and setup guidance.
- Clarified the `bass.dll` runtime requirement and BASS licensing notes.
- Preserved fork attribution to the original ECM project by BttrDrgn.

[v0.5.0-alpha]: https://github.com/RCompanyX/ecm/releases/tag/v0.5.0-alpha
[v0.5.1-alpha]: https://github.com/RCompanyX/ecm/releases/tag/v0.5.1-alpha
[v0.5.2-alpha]: https://github.com/RCompanyX/ecm/releases/tag/v0.5.2-alpha
