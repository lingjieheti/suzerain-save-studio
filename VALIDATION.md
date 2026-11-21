# v0.13.0 Preview — Windows validation

The Windows x64 archive contains the portable editor and its optional in-game plugin. The editor was built with MinGW-w64 GCC 13.2.0. Its version resource reports 0.13.0 and it runs independently of the plugin.

## Native editor and files

- Exercised both embedded campaigns through the startup picker. Exported a Sordland sample with its budget changed from 10 to 15 and reopened the output.
- Used multi-word Chinese and raw-key queries on common and advanced pages, including an empty result. Verified selection memory, invalid-input handling, individual reversion, undo/redo and the counts for linked Rizia resource caps.
- Inspected the wide and compact editor layouts in Chinese and English at 200% Windows scaling. The editor screenshots use embedded examples. The save library screenshot uses six synthetic campaign files. Its search, empty results, keyboard focus and campaign choices were exercised in a native Windows dialog.
- Opened a scratch copy of a synthetic save through the executable, applied a budget increase and confirmed that only the expected numeric bytes changed. The pre-save backup matched the original byte for byte. Restoring through the Backups page reproduced the original and kept the modified version as a second backup.
- All 30 synthetic file I/O and setup checks and 78 native editor checks passed, including external modifications, concurrent saves, campaign isolation, the sample overwrite guard, existing plugin handling and theme changes during an active edit.

## Appearance

- Selected all four themes from the actual Windows popup. Exercised both the save picker and editor, including keyboard selection, cancellation, active search, a pending numeric edit, an invalid draft, undo history and restart persistence. Wide and compact layouts were inspected at 200% scaling. The four-theme comparison shows the same embedded Rizia example.
- The native test suite checks localized theme names in all 11 interface languages and exercises missing, invalid and valid preferences in an isolated user registry. The game panel tests independently read and write the same Windows DWORD and reload it when the panel opens.

## In-game plugin

- Compiled the .NET 6 IL2CPP plugin against a local Steam installation and BepInEx 6.0.0-be.755 with zero warnings and zero errors. The release includes only the project's plugin DLL.
- Ran 44 simulated campaign checks on field selection, resource limits, stale sessions and readback after writing. Another 25 offline checks covered panel display, keyboard input, confirmation, refresh and live appearance changes. The panel image in the guide is an offline rendering with synthetic values.
- Exercised the installer's present/missing-DLL dialogs. From the packaged layout, selected a synthetic game executable through the Windows file chooser and verified the installed plugin bytes.
- Live writes and HUD refresh inside a running campaign still need an in-game test.

## Automated and read-only coverage

| Suite | Result |
| --- | --- |
| Parser and renderer | 265 checks passed |
| Session history and patch format | 158 checks passed |
| Save discovery | 23 checks passed. The filesystem symlink test was skipped locally because the account could not create a test symlink. |
| Interface localization | 4,345 checks passed |
| Curated labels | 452 translations across four western languages verified, plus 1,489 checks for eastern languages, Russian and Turkish |
| Display values | 90,256 checks across 11 languages |
| Advanced Chinese names | 24,617 checks across 4,097 fields |
| Local game saves | 233 read-only checks covering 15 Sordland and 6 Rizia files, with zero discovery errors |

For the read-only run, edits and patch round trips were rendered in memory. SHA-256 values before and after matched for every original player save. The published repository includes synthetic fixtures and key/type inventories. Actual player saves are excluded.

The observed game versions were 3.1.0.1.175 and 3.1.0.1.137. Edited saves have not been loaded inside the game. In-game event outcomes, unobserved formats and Wine are unverified. The executable is unsigned.

## Reproduce

Build with `build.ps1 -CompilerKind GCC`, then run `test.ps1 -Compiler <g++.exe>`. Pass `-SaveDirectory <folder>` for the optional read-only check. The default suites use synthetic fixtures. The application's dependency notices are available from Help and in the source tree.

Run `dotnet run --project tests/live/LiveChecks.csproj -c Release` with a .NET 6 SDK to reproduce the simulated campaign suite. To build the actual plugin against your installation, follow the [live panel guide](live/README.zh-CN.md).
