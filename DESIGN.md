# Design

## Source of truth
- Status: Active
- Last refreshed: 2026-08-12
- Primary product surfaces: YUP Standalone, VST3, AUv2 editor
- Evidence reviewed: fractional-delay and feedback-comb references plus the nine-effect Digital Harsh Noise UI survey

## Brand
- Personality: fractured timing terminal; violent but controlled.
- Trust signals: visible values, stable parameter order, honest input/output meters.
- Avoid: color accents, gradients, glow, rounded cards, fake telemetry, flashing.

## Product goals
- Turn input into spatially torn, drifting repeats while remaining bounded and automatable.
- Make all seven controls visible at once.
- Preserve hosted silence and isolate Standalone audition state.
- Non-goals: tape-delay nostalgia, MIDI instrument behavior, nondeterministic rendering.

## Information architecture
- Header: `SHATTERDELAY` and one-line warning.
- Standalone-only runtime strip: audition source and quantized input/output meters.
- Main surface: Time, Feedback, Shatter, Damping, Drift, Mix, Output in one row.

## Design principles
- Monochrome means state through inversion, density, and outline—not hue.
- Pixel character comes from integer geometry, grid, scanlines, and stepped meters.
- Host automation semantics take precedence over decorative control replacement.

## Visual language
- Color: `#000000` to `#FFFFFF` grayscale only.
- Typography: YUP native fallback; uppercase labels and stable numeric value labels.
- Layout: 960x540 logical canvas, square edges, no elevation.
- Motion: 30 Hz decaying meters only; no stochastic or full-panel animation.

## Components
- ParameterGridEditor with YUP rotary sliders and host gestures.
- Seven-column single-row parameter grid.
- Standalone-only audition buttons plus 32-step rectangular meters.

## Accessibility
- White or muted-gray text on black with high contrast.
- No flashing; meter motion is functional and low amplitude.
- Values remain textual; controls retain native YUP interaction behavior.

## Responsive behavior
- Preferred size: 960x540, resizable with aspect ratio preserved.
- Seven controls remain in one row; control diameter shrinks before labels disappear.

## Interaction states
- Hosted silence stays silent.
- Standalone audition is runtime-only and is not serialized.
- Disabled audition produces silence; meters show processor atomics only.

## Implementation constraints
- C++20 and YUP; no new runtime dependency or external asset.
- Integer-aligned `fillRect` rendering; no ellipse, gradient, shadow, or rounded meter.
- Audio thread performs no allocation, locks, I/O, logging, or UI calls.
- App/plugin ID `jp.ehl.shatterdelay`; vendor `ehl_`; AU `ShDl` / `EHL1`.

## Verification
- Seven stable parameter IDs and state magic `SHD1`.
- Deterministic delay arrival/tail, parameter effect, extremes, denormals, and finite bounds.
- Hosted silence/state bridge and Standalone audition/meter isolation.
- Standalone/VST3/AU build and staged artifacts on macOS; Standalone/VST3 on Windows CI.

## Open questions
- [ ] Evaluate maximum Shatter feedback texture in several hosts after the first public listening round.
