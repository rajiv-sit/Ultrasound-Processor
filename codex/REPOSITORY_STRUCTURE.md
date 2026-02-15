# Ultrasound-Processor Repository Structure

```text
Ultrasound-Processor/
|- apps/
|  |- replay_runner.cpp
|  |- legacy_capture_convert.cpp
|  \- imgui_visualizer.cpp
|- bindings/
|  |- imgui_impl_glfw.cpp
|  |- imgui_impl_glfw.hpp
|  |- imgui_impl_opengl3.cpp
|  |- imgui_impl_opengl3.hpp
|  \- imgui_impl_opengl3_loader.hpp
|- cmake/
|  |- ProjectOptions.cmake
|  \- profiles/
|     |- host-debug-vs2022
|     |- host-release-vs2022
|     \- build-release-vs2022
|- codex/
|  |- EXTRACTION_PLAN.md
|  |- ULTRASOUND_BOUNDARY.md
|  |- REPO_INVENTORY.md
|  |- ULTRASOUND_FILEMAP.md
|  |- ULTRASOUND_VISUALIZATION_FILEMAP.md
|  |- DEPENDENCIES.md
|  |- ARCHITECTURE.md
|  |- HLD.md
|  |- DD.md
|  |- FUNCTIONAL_REQUIREMENTS.md
|  |- NON_FUNCTIONAL_REQUIREMENTS.md
|  |- IMPLEMENTATION_CHECKLIST.md
|  |- BUILD_AND_RUN_VERIFICATION.md
|  |- CURRENT_STAGE.md
|  |- RELEASE_READINESS.md
|  \- REPOSITORY_STRUCTURE.md
|- configs/
|  |- default_ultrasound_processor.ini
|  \- vehicle_profile_reference.ini
|- data/
|- include/ultrasound/
|  |- config.hpp
|  |- config_io.hpp
|  |- diagnostics.hpp
|  |- error.hpp
|  |- processor.hpp
|  |- replay.hpp
|  |- runtime.hpp
|  |- types.hpp
|  |- vehicle_geometry.hpp
|  \- visualizer.hpp
|- replay/
|  |- sample_input.csv
|  \- generated_from_legacy.csv
|- src/
|  |- core/
|  |  \- processor.cpp
|  |- io/
|  |  |- config_loader.cpp
|  |  |- replay_source.cpp
|  |  \- runtime_stub.cpp
|  \- visualization/
|     \- imgui_visualizer.cpp
|- tests/
|  |- test_processor.cpp
|  |- test_config_loader.cpp
|  |- test_replay_source.cpp
|  \- test_runtime_stub.cpp
|- visualization/
|  \- imgui.ini
|- CMakeLists.txt
|- conanfile.py
\- README.md
```

## Standalone Boundary
- No source/include/library paths are consumed from parent repositories.
- Visualizer dependencies are resolved via Conan (`glfw`, `glew`, `imgui`, `opengl`) and local `bindings/`.
- Public artifacts are limited to replay CSV and sanitized configs in this repository.

## Repository Status (2026-02-15)
- Structure is finalized for RC1 release.
- Standalone boundary is enforced in build scripts and dependency graph.
- Added release governance docs:
  - `codex/CURRENT_STAGE.md`
  - `codex/RELEASE_READINESS.md`
