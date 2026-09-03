# Visual Studio Build

## Prerequisites

- Visual Studio 2022 with the Desktop development with C++ workload.
- Qt 6.8.3 `msvc2022_64`, registered in Qt Visual Studio Tools as `6.8.3_msvc2022_64`.
- VTK 9.4.2 installed under `D:\VTK9.4.2\install`.
- OpenCASCADE 7.7.0 installed under `D:\OCCT7.7.0\Install`.
- Qt Visual Studio Tools for Visual Studio 2022.

The project is x64-only and uses the C++17 language standard.

## Open And Build

1. Open `Practice_Modeling.sln`.
2. Select `x64` and either `Debug` or `Release`.
3. Build the solution.
4. Press `F5` to run `Practice_Modeling`.

If Qt Visual Studio Tools is not installed, install it first and register the Qt kit above. The `QtInstall` value in `Practice_Modeling.vcxproj` can also be changed to the exact Qt installation path.

## Runtime DLLs

The executable needs Qt, VTK, and OpenCASCADE DLLs at runtime. The project uses the following directories:

- `D:\Qt\6.8.3\msvc2022_64\bin`
- `D:\VTK9.4.2\install\bin`
- `D:\OCCT7.7.0\Install\win64\vc14\bin`

For a deployable folder, run `scripts\build_release.bat`. It builds the release target and runs `windeployqt`; keep the VTK and OpenCASCADE `bin` directories on `PATH` when launching the executable.

Visual Studio builds now write to `vsbuild\Debug` and `vsbuild\Release`.

## Command Line Build

From a regular PowerShell prompt:

```powershell
.\scripts\build_debug.bat
.\scripts\build_release.bat
```

The scripts locate the latest installed Visual Studio instance through `vswhere.exe`, so they do not depend on the default `C:\Program Files` installation path.
