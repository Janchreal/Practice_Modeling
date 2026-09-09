# Presentation Layer

`presentation/` contains Qt dialogs, UI forms, and the main-window adapter for
the modeling application. Rendering, geometry, and document persistence are
owned by other modules.

## Naming Rules

- Use `snake_case` for file names.
- Use `main_window.*` for the main `Widget` window and
  `main_window_<feature>.cpp` for split implementation units.
- Keep dialogs under `dialogs/<feature_area>/` using
  `<feature>_dialog.{h,cpp,ui}` for dialog classes and their forms.
- Use `mirror_view_*` for auxiliary mirrored VTK windows and their shared
  rendering state.
- Use `main_menu_builder.*` for menu and ribbon construction helpers.
- Generated `ui_*.h` files come from Qt Designer forms and must not be edited
  manually.

## Responsibility Boundaries

- `main_window/`: window construction, action wiring, and the current
  transitional adapter between Qt, commands, and the renderer.
- `main_window_<feature>.cpp`: one focused responsibility for the main window.
- `dialogs/`: parameter input, validation, and dialog signals only.
  Subdirectories group dialogs by feature area: primitives, extrude/revolve,
  modification, boolean, coordinates, sketch, pattern, tools, history, and
  common shared dialogs.
- Geometry algorithms belong in `geometry/`.
- Document and history records belong in `application/history/`.
- Commands belong in `application/commands/`.
- VTK view-window lifecycle belongs in `viewport/`.
