# Groove Forge

`Groove Forge` is a Linux-native generative groovebox MVP built with `Qt5 Widgets`, `JACK`, and plain C++17.

## What it does

- 16-step sequencer with kick, snare, hat, and bass tracks
- Density controls per track
- Swing, tempo, mutation depth, and auto-mutation bar count
- Live regeneration and pattern mutation
- Internal drum and bass synthesis with no external samples
- JACK audio output

## Layout

- `src/app`: application orchestration
- `src/audio`: JACK transport and render loop
- `src/core`: shared musical data structures
- `src/dsp`: synth voices
- `src/generation`: pattern generation and mutation rules
- `src/ui`: Qt widgets and layout
- `tests`: non-UI verification

## Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Run

Start JACK or PipeWire's JACK shim first, then launch:

```bash
./build/groove_forge
```
# grooveforge
