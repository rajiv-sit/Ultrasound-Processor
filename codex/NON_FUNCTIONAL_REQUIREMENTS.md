# Non-Functional Requirements

## Determinism
NFR-1 Replay runs shall be deterministic for fixed input/config and toolchain.
NFR-2 Numeric parity with legacy shall be validated within defined tolerances per signal group.

## Performance
NFR-3 Average per-frame processing latency shall be <= 10 ms in replay baseline.
NFR-4 p99 per-frame latency shall be <= 20 ms under nominal load.
NFR-5 CPU and memory usage shall be observable via diagnostics counters.
NFR-5a Visualizer shall sustain interactive playback at >= 15 FPS on baseline development hardware.

## Reliability
NFR-6 Invalid frames shall be dropped safely with explicit reason codes.
NFR-7 Out-of-order timestamps shall not corrupt processor state.
NFR-8 Defaults shall be safe and conservative when config keys are missing.

## Portability
NFR-9 Core library shall compile on modern C++ toolchains (minimum C++20).
NFR-10 Platform-specific code shall be isolated to IO adapters.
NFR-10a Visualizer platform-specific OpenGL/window code shall be isolated in `ultrasound_visualizer` target.

## Maintainability
NFR-11 Build shall be reproducible via Conan profiles and lockfiles.
NFR-12 Public API and configuration schema shall be versioned.
NFR-13 Architecture and design docs shall track implementation changes.

## Quality Gates
NFR-14 Unit and integration tests shall run in CI.
NFR-15 Static analysis and formatting checks shall be part of CI.
NFR-16 Minimum test coverage threshold shall be defined before M4 sign-off.

## NFR Status (2026-02-15)
- Determinism: validated by repeatable replay outputs and dedicated deterministic tests.
- Reliability: guarded error paths and drop accounting implemented and covered.
- Portability boundary: core/io/visualizer separation is in place; VS2022 profile pinning used for stable builds.
- Quality gate: minimum coverage requirement satisfied (`82.81% >= 80%`).
- CI/static analysis are not yet mandatory gates in this RC1 drop.

