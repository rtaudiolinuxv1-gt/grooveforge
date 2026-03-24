# RTAudioSeq (RTAudioLinux)

`RTAudioSeq` is a Linux-native generative soundfont sequencer built with `Qt5 Widgets`, `JACK`, `FluidSynth`, and plain C++17.

## What it does

- Variable-length step sequencer with dynamic instruments
- SF2, sample, and MIDI instrument routing
- Swing, tempo, mutation depth, and auto-mutation bar count
- Live regeneration and pattern mutation
- JACK audio output and offline WAV export

## Layout

- `src/app`: application orchestration
- `src/audio`: JACK transport and render loop
- `src/core`: shared musical data structures
- `src/generation`: pattern generation and mutation rules
- `src/ui`: Qt widgets and layout
- `tests`: non-UI verification

## Build

```bash
mkdir build && cd build
cmake ..
```

## Run

Start JACK or PipeWire's JACK shim first, then launch:

```bash
./RTAudioSeq
```

## License

This project is licensed under `GPL-3.0-only`. See [LICENSE](/home/jim/BUILD/LICENSE).
