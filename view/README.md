# View Layer

`view/` contains Qt windows, dialogs, UI forms, and the presentation-side
interaction code for the modeling application.

## Naming Rules

- Use `snake_case` for file names.
- Use `main_window.*` for the main `Widget` window and
  `main_window_<feature>.cpp` for split implementation units.
- Use `<feature>_dialog.{h,cpp,ui}` for dialog classes and their forms.
- Use `mirror_view_*` for auxiliary mirrored VTK windows and their shared
  rendering state.
- Use `main_menu_builder.*` for menu and ribbon construction helpers.
- Generated `ui_*.h` files come from Qt Designer forms and must not be edited
  manually.

## Responsibility Boundaries

- `main_window.*`: window construction, shared state, and public UI-facing
  declarations.
- `main_window_<feature>.cpp`: one focused interaction or presentation
  responsibility for the main window.
- `*_dialog.*`: parameter input, validation, and dialog signals only.
- `mirror_view_*`: mirror-window presentation and renderer binding.
- Geometry algorithms, model data, and commands belong in `core/`, `model/`,
  and `controller/` respectively.

