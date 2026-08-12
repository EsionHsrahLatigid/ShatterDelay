# ShatterDelay

ShatterDelay is a YUP stereo delay built as a Digital Harsh Noise performance weapon. Fractional stereo taps, cross-feedback, loop damping, slow delay drift, activity-gated deterministic fracture bursts, and bounded wavefolding turn an input into torn, unstable-looking repeats without making the render nondeterministic. Hosted builds preserve silence; Standalone adds an audition source and meters only at compile time.

## Identity and formats

- App/plugin ID: `jp.ehl.shatterdelay`
- Vendor: `ehl_`; AU manufacturer: `EHL1`; AU subtype: `ShDl`
- Version: `0.1.0`
- macOS: Standalone, VST3, AUv2
- Windows: Standalone, VST3
- Stereo effect, no MIDI

## Parameters

- `Time`: nonlinear 8–948 ms base-delay range.
- `Feedback`: bounded cross-channel regeneration.
- `Shatter`: wavefold depth and sparse fracture-event intensity.
- `Damping`: one-pole filtering inside the repeat path.
- `Drift`: slow fractional-delay modulation with offset stereo motion.
- `Mix`: dry/wet blend.
- `Output`: final gain before the bounded safety stage.

## Research basis

The interpolated delay reads follow the principles in [Physical Audio Signal Processing: Fractional Delay Filters](https://www.dsprelated.com/freebooks/pasp/Fractional_Delay_Filters.html). Feedback topology and stability are grounded in [Comb Filters](https://www.dsprelated.com/freebooks/pasp/Comb_Filters.html). ShatterDelay deliberately adds cross-channel regeneration, activity-gated deterministic fracture impulses, and bounded folding as product synthesis; those choices are not claims made by the references.

## Build and artifacts

Clone with `--recurse-submodules`, or initialize the shared [yup-ehl-design-module](https://github.com/EsionHsrahLatigid/yup-ehl-design-module) before configuring:

```sh
git submodule update --init
```

```sh
cmake --preset engine-debug
cmake --build --preset engine-debug --parallel
ctest --preset engine-debug --output-on-failure

cmake --preset plugin-release
cmake --build --preset plugin-release --parallel
ctest --preset plugin-release --output-on-failure
```

Human-facing products are staged under `artifacts/plugin-release/<platform-arch>/` in `standalone/`, `vst3/`, and macOS `au/`. `build/` is internal compiler state.

## CI and release

The caller workflows pin `EsionHsrahLatigid/yup-actions` to a full commit SHA. CI runs deterministic tests and Release product staging on macOS arm64 and Windows x64, then creates checksummed latest ZIP artifacts. A `v*` tag promotes artifacts from the successful `main` CI run for that exact commit without rebuilding.

## Safety contract

The audio callback allocates no memory and performs no locks, I/O, logging, or UI work. Parameters and non-finite input are sanitized; feedback is kept below unity; fracture events require signal activity; output stays finite within `+/-0.98`. Delay-arrival, feedback-tail, parameter-effect, deterministic-render, extreme-value, hosted-silence, state, and Standalone-audition tests cover the contract.
