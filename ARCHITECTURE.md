# Architecture

The source tree is organized by responsibility rather than by file type:

```text
src/
  app/                         process entry point
  common/                      shared enums and small cross-layer value types
  application/
    command/                   command base types bound to application ports
    commands/                  user actions with undo/redo semantics
    history/                   command history and history-state adapters/helpers
    ports/                     interfaces used by commands
  domain/
    features/                  feature parameters and feature enums
    sketch/                    sketch model
  geometry/
    boolean/                   OpenCASCADE boolean algorithms
    extrusion/                 extrusion algorithms and recipe helpers
    modification/              fillet, chamfer, and hollow algorithms
    pattern/                   pattern algorithms
    placement/                 shared origin and axis placement helpers
    primitives/                primitive solid builders
    revolution/                revolution algorithms
    sketch/                    geometric queries for sketches
    topology/                  sub-shape references and resolution
  interaction/
    coordinates/               datum axes/planes and default reference coordinate system interactions
    handles/                   handle specifications
    selection/                 selection-related queries and picker binding refresh
    tools/                     point snapping and vector picking
  rendering/
    adapters/                  OpenCASCADE to VTK conversion
    model/                     VTK model state and model presentation factories
    pipeline/                  scene, appearance, and highlight renderers
    handles/                   VTK handle geometry
  viewport/
    coordinates/               view triad and work coordinate system viewport behavior
    main_view/                 main VTK viewport, mouse interactor, picking entry points, and camera helpers
    mirror/                    auxiliary VTK view windows
  presentation/
    dialogs/                   Qt dialogs and Designer forms, grouped by feature area
      boolean/                 boolean operation dialogs
      common/                  shared utility dialogs
      coordinates/             datum and coordinate-system dialogs
      extrude_revolve/         extrude and revolve dialogs
      history/                 history list item widgets
      modification/            fillet and chamfer dialogs
      pattern/                 pattern dialogs
      primitives/              primitive parameter dialogs
      sketch/                  sketch creation and sketch tool dialogs
      tools/                   tool-specific dialogs
    features/                  feature-specific UI workflows around Widget
    main_window/               main window assembly and transitional adapters
  infrastructure/
    serialization/             JSON and BREP persistence
```

## Dependency Rules

- `domain` must not depend on `QWidget`, Qt dialogs, VTK actors, or renderer state.
- `geometry` may depend on OpenCASCADE, but not on Qt Widgets or presentation code.
- `application` may depend on domain and geometry ports, but must not depend on concrete widgets.
- `interaction` produces selections, references, and interaction state; it does not mutate the document directly.
- `rendering` owns VTK resources and rendering adapters; it does not own feature parameters.
- `viewport` owns view-window lifecycle and renderer binding.
- `presentation` may depend on application, interaction, rendering, and viewport.
- `infrastructure` implements persistence and external-library adapters.

## Transitional Constraints

`ModelingHistory` and `ModelDocument` live under `application/history`. `ModelingHistory` owns document metadata, parameters, and feature relationships. OCC runtime geometry is held by `ModelGeometryStore`; VTK runtime resources are held by `ModelRenderStore`. Both stores are keyed by the stable record ID assigned by `ModelDocument`.

`AxisPlacement` is the shared geometry value object for origin, axis, reverse direction, and custom vector placement. `PrimitiveBuildRequest` groups a primitive type, its three persisted parameter slots, and its placement for primitive creation and regeneration. Code that copies placement into or out of `ModelingHistory`, `ModelHistorySnapshot`, or feature recipes should go through `application/history/modeling_history_placement.h`.

Primitive-specific rebuild code should go through `application/history/modeling_history_primitives.h`. History index remapping for insert/remove operations lives in `application/history/modeling_history_index_remap.h`.

`ShapePresentationFactory` owns the repeated OCC-to-VIS-to-VTK presentation setup for solid models: meshing, `IVtkOCC_Shape`, shared shape data source, shaded actor, VIS highlight actor, and cached polydata. `ModelDisplayStyle` owns reusable VTK appearance rules for solid, sketch, hover, feature-pick, translucent, and boundary-outline states. `Widget` keeps the transitional responsibility for inserting those actors into renderers and connecting them to document history.

`ModelHistorySnapshot` lives under `application/history` because it is undo/restore protocol data, not a domain entity. It still carries an OCC shape for the current undo/restore path; that is an intentional transitional dependency. Future commands should use a serializable geometry snapshot or a domain-level regeneration input instead.

`geometry/topology` creates and resolves references against an explicitly supplied `TopoDS_Shape`. Application contexts are responsible for obtaining the parent shape, so geometry does not depend on `ModelingCommandPort`.

`ModelingCommandPort` is an application port, but `Widget` still implements it. This is intentionally transitional. New commands must receive the port as `context` and must not use a concrete `Widget` type.

`ModelDocument` owns records only. JSON and BREP serialization lives in `infrastructure/serialization`.

## Main-Window State Boundaries

`Widget` is still the Qt orchestration shell, but its long-lived state is split
into small compatibility boundaries so feature code does not need to add more
fields to the window class:

- `interaction/selection/selection_window_state.h` owns selection modes,
  selected history indices, sub-shape highlights, and transient hover state.
- `interaction/selection/shape_picker_binding_service.h` owns the generic
  picker-to-renderer rebinding operation. The window supplies only the active
  context preparation and data-source refresh callbacks.
- `viewport/main_view/viewport_window_state.h` owns the main VTK widget,
  renderer, picker, interactor, and render pipeline instance.
- `viewport/main_view/view_navigation_window_state.h` owns standard-view
  camera transition state.
- `presentation/main_window/coordinate_window_state.h` owns the view triad,
  work coordinate system, and reference coordinate system actors.
- `presentation/main_window/dialog_window_state.h` owns dialog pointers and
  datum preview actors, all initialized to null.
- `presentation/main_window/document_window_state.h` owns file-session flags,
  recent files, auto-recovery timer, and recent-file controls.
- `presentation/main_window/model_document_window_state.h` owns the model
  document, runtime geometry/render stores, command manager, and history
  regeneration guard.
- `rendering/model/model_rendering_window_state.h` owns the OCC-to-VTK
  conversion adapter used while creating model presentations.
- `presentation/main_window/ribbon_window_state.h` owns Ribbon widget
  references and sketch-environment mode state.
- `presentation/main_window/feature_interaction_window_state.h` owns the
  transient extrusion, revolve, fillet, chamfer, and feature-preview state.
- `presentation/main_window/pattern_window_state.h` owns pattern-dialog and
  pattern-preview state.
- `presentation/main_window/primitive_interaction_window_state.h` owns the
  interactive cuboid construction state and dimension overlays.
- `presentation/main_window/sketch_window_state.h` owns sketch mode, sketch
  editing, preview, and sketch-dialog state.
- `presentation/main_window/vector_snap_window_state.h` owns point snapping,
  vector picking, and vector-handle state.

These classes deliberately contain state only. Their methods remain on
`Widget` while the existing Qt signal/slot surface is migrated. New behavior
should be implemented in the corresponding geometry, application, interaction,
rendering, viewport, or feature controller module and exposed through a narrow
adapter rather than growing `Widget` with another unrelated responsibility.

## Migration Status

The directory migration, primitive base hierarchy, render-layer split, runtime
state extraction, and Visual Studio/qmake project synchronization are complete.
The requested structural refactor is therefore complete: primitive creation,
modification, rendering, tools, coordinates, dialogs, boolean operations, and
viewport/window code each have an explicit module boundary.

There is one explicitly tracked follow-up layer, not unfinished directory
work. `Widget` still implements `ModelingCommandPort`, and some
viewport/interaction implementation units still include `main_window.h` to
preserve the current signal/slot and input behavior. Removing that coupling is
a separate interface migration that should be done feature by feature with
interaction tests. It is technical debt for the next iteration, not a blocker
for this refactor's architecture target.
