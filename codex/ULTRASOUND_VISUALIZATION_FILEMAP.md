# Ultrasound Visualization File Map (Legacy -> Standalone)

## Primary Legacy USS Visualizer Slice
- `datasources/sensordrivers/ValeoSensorDriver/dev/include/VSDDev/ValeoSensorDriverVisualization.h`
  - USS visualization state, ImGui controls, and world/local draw entrypoints.
  - Render toggles for gridmap, signal-ways, static/dynamic features, and post-processed detections.

- `datasources/sensordrivers/ValeoSensorDriver/dev/src/ValeoSensorDriverVisualization.cpp`
  - ImGui panel: `General` + `Post processing USS Signal Ways Options`.
  - OpenGL draw routines for signal-ways, detections, ellipses, gridmap overlays.
  - Trace plotting and detection category counters.

- `datasources/sensordrivers/ValeoSensorDriver/dev/src/ValeoSensorDriverDevInterface.cpp`
  - Visualization registration and per-frame data injection.
  - Bridges processed signal-way outputs into visualizer setters.
  - Loads visualization-affecting INI fields (`groupID`, `method`).

## Shared Visualization Framework Dependencies
- `framework/DAT_framework/LibAPI/VisualizationBase.h`
  - Base class for OpenGL window/context, ImGui lifecycle, draw utilities.
  - Includes ImGui+GLFW+OpenGL backend headers.

- `framework/DAT_framework/LibAPI/VisualizationBase.cpp`
  - Context/window creation via GLFW and GLEW.
  - ImGui context creation, frame begin/end, viewport rendering.

- `framework/DAT_framework/LibAPI/VisualizationBaseFunctions.h`
  - ImGui convenience controls (`comboBox`, map-based combo, radio groups).

- `framework/DAT_framework/LibAPI/DatWorldVisualization.h`
  - Main docking host and registration API (`addWorldVisualization`).

- `framework/DAT_framework/LibAPI/DatWorldVisualization.cpp`
  - Main development environment window, dockspace, debug menu, view control.
  - Invokes child visualizer pre-display and draw callbacks each frame.

- `framework/DAT_framework/LibAPI/Shader.h`
  - OpenGL shader variants and colormap plumbing used by Valeo visualizer.

- `framework/DAT_framework/LibAPI/TimeTracePlot.h`
  - Historical trend plots used in Valeo detection count panel.

## Vendor ImGui/Backend Sources Used in Legacy
- `vendor/imgui/imgui.cpp`
- `vendor/imgui/imgui_draw.cpp`
- `vendor/imgui/imgui_widgets.cpp`
- `vendor/imgui/imgui_demo.cpp`
- `vendor/imgui/imgui_impl_glfw.cpp`
- `vendor/imgui/imgui_impl_opengl3.cpp`
- `vendor/imgui/imgui.h`
- `vendor/imgui/imgui_impl_glfw.h`
- `vendor/imgui/imgui_impl_opengl3.h`
- `vendor/imgui/LICENSE.txt`

## Standalone Mapping
- New extracted implementation:
  - `bindings/imgui_impl_glfw.cpp/.hpp` (backend bridge, LiDARProcessor-style)
  - `bindings/imgui_impl_opengl3.cpp/.hpp` + loader header
  - `include/ultrasound/visualizer.hpp`
  - `src/visualization/imgui_visualizer.cpp`
  - `apps/imgui_visualizer.cpp`
  - `visualization/imgui.ini`
- Build wiring:
  - `CMakeLists.txt`: `ultrasound_visualizer` and `uss_imgui_visualizer`
  - `conanfile.py`: Conan deps `imgui`, `glfw`, `glew`, `opengl/system` (+ toggle `with_visualizer`)

## Visualization Filemap Status (2026-02-15)
- Legacy visualization mapping to standalone visualizer implementation is complete.
- Implemented with Conan-managed dependencies and in-repo backend bindings.
- Exact rendering parity in all corner cases remains ongoing.

