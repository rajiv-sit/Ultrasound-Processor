# Implementation Checklist

## M0 Buildability
- [x] Create standalone repository layout.
- [x] Add Conan recipe and profiles.
- [x] Add CMake targets: `ultrasound_core`, `ultrasound_io`, `ultrasound_apps`.
- [x] Build on Windows with no absolute paths.

## M1 Deterministic Replay Path
- [x] Implement replay input adapter.
- [x] Implement state interpolation queue.
- [x] Implement replay-format -> static/signal-way conversion.
- [x] Implement signal-way post-processing (tracing/FOV/ellipse + clustering).
- [ ] Add golden replay fixtures and parity tests.

## M1.5 Visualization Parity Path
- [x] Extract legacy ImGui backend sources into standalone repository.
- [x] Add `ultrasound_visualizer` target and `uss_imgui_visualizer` app.
- [x] Mirror major visualizer overlay semantics (signal-ways, gridmap, static/dynamic feature details).
- [ ] Add visualizer regression screenshots for deterministic replay frames.

## M2 Real-time Path
- [ ] Implement live UDP ingest adapter.
- [x] Add monotonic timestamp guard and drop accounting.
- [x] Add output publishing adapter hooks.

## M3 Performance and Profiling
- [x] Add per-stage timing and frame counters.
- [ ] Validate latency budgets with representative datasets.
- [ ] Document CPU/memory baseline.

## M4 Quality Gates
- [x] Unit tests for converters and post-processing geometry.
- [x] Integration tests for replay end-to-end output.
- [ ] CI pipeline (configure, build, test, lint).
- [ ] Static analysis (`clang-tidy` and/or `cppcheck`).
- [x] Coverage threshold and report publishing.

## Migration Controls
- [x] Preserve legacy key names where compatibility mode is enabled.
- [x] Preserve packet semantics for downstream integration.
- [ ] Keep algorithm logic unchanged before refactoring.

## Checklist Status (2026-02-15)
- M0: complete.
- M1: complete for replay path; golden-vector parity extension remains open.
- M1.5: complete for core visualizer features; exact legacy visual parity remains iterative.
- M2: partial (callback/runtime abstraction only; full live ingest pending).
- M3: partial (timings implemented; profiling baselines pending).
- M4: partial-to-complete (unit tests + coverage gate done; CI/static analysis pending).
- This is sufficient for RC1 release with documented limitations.
