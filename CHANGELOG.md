# Changelog

All notable changes to Yano Swing are documented here. Format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and versioning
follows [Semantic Versioning](https://semver.org/) adapted for a pre-1.0
beta:

- **PATCH** (0.x.y): bug fixes only, no behavior/feature changes
- **MINOR** (0.x.0): new features or notable user-facing changes
- **MAJOR** (1.0.0+): first stable release, then breaking changes only

## [0.1.0] - 2026-07-17

### Added
- First public beta build. One-knob MIDI groove processor for amapiano
  production: off-grid 16th notes are delayed for a laid-back feel,
  on-grid 16th notes pass through untouched, matching note-offs always
  carry the same delay as their note-on, all driven by a single Amount
  macro.
- Syncs to host tempo/playback position via the DAW's playhead.
- Builds as VST3, AU (MIDI processor, passes `auval`), and Standalone on
  macOS and Windows.
