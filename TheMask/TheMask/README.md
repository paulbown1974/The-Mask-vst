# The Mask (VST3)

An effect plugin that finds resonant / "bad" frequencies in a sound and masks them.
Modes: Live, Learn (analyse a few seconds, freeze a curve saved with the project), Both.

## Build on Windows
1. Install Visual Studio 2022 (workload "Desktop development with C++") and CMake.
2. In a terminal in this folder:

       cmake -S . -B build -G "Visual Studio 17 2022" -A x64
       cmake --build build --config Release --target TheMask_VST3

   (First run downloads JUCE 8.0.6. To use your own copy: `-DTHEMASK_JUCE_DIR=C:\path\to\JUCE`.)
3. Copy `build\TheMask_artefacts\Release\VST3\The Mask.vst3` to `C:\Program Files\Common Files\VST3` and rescan in your DAW.

No Visual Studio? Push this folder to GitHub: the workflow in `.github/workflows` builds the VST3 and offers it as a download.

## Status (milestone 1: engine + GUI shell)
- Working: Mask engine (all modes, controls, delta, Mix, bypass), 12 factory presets (settings only), basic limiter, GUI.
- Preview only: Glue panel (dial and toggles move, but there is no sidechain analysis/suggestions yet).
- Limiter is a first pass: no look-ahead, no true-peak.
- Latency is 2048 samples (about 42.7 ms at 48 kHz); the plugin reports it so the DAW compensates.
- Built and tested on Linux only. Engine tests: `TheMaskEngineTests` target. Not yet tried on Windows or in a DAW.

## Next
Glue DSP, better limiter, preset browser with learned curves.
