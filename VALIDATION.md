# v0.16.0 Preview — Windows validation

The Windows x64 package pairs the 0.16.0 portable editor with the existing 0.15.1 optional in-game plugin. The editor uses MinGW-w64 GCC 13.2.0.

## Search and backups in 0.16.0

- Checked combined campaign, slot-type and turn queries in the native picker, including turn 3 versus 13. The fixture suite also covers digits in filenames, the five game slot prefixes and localized turn/type terms.
- Exercised the executable extracted from the ZIP at picker sizes of 535×440 and 1010×624 and editor sizes of 680×485 and 1160×770 logical pixels. Search, clear, preview, and individual reversion worked using seven synthetic saves. All seven source files retained their original bytes.
- Reproduced the failed guide lookup in the native editor before fixing it. Copied Lua and JSON-escaped assignments now locate the exact field. Ordinary text still returns multiple results. A Rizia sample with an energy stock, turn total and income modifier exercises their ordering and the return to alphabetical browsing after clearing the search.
- Tested similar slot names, filenames containing timestamps, manually named backups and older backup names. Backups can be opened and saved as new copies outside the backup directory. Overwriting files in that directory is blocked.
- Read 15 Sordland and 6 Rizia saves and ran 233 checks on parsing, in-memory edits, export and undo. The 73 simulated live-state and 218 panel checks passed. The bundled companion DLL matches the previous release byte for byte.
- The local build passed 201 native editor checks and 39 synthetic file I/O checks. The discovery suite skipped its symbolic-link creation case because the local account could not create one. Two packages produced from the same compiled files had identical SHA-256 digests.

## Fjord in 0.15.1

The desktop resource strip was checked with both embedded campaigns, 11 languages and three window sizes (1160×770, 840×650 and 680×630). All 396 text-width checks passed. Native Windows screenshots also confirmed the longer French and Russian Sordland economy labels at compact width.

The desktop and optional live panel now start in Fjord when the Windows appearance preference is missing or invalid. The four saved theme IDs retain their meanings. Existing explicit selections, including Paper, are covered by the preference tests. The live panel passed 73 simulated game-state checks and 218 panel checks. The live panel image uses synthetic values rendered from the production drawing code.

## Visual update in 0.15.0

- Tested the Windows workspace with the built-in Rizia sample and the picker with six synthetic saves. Checked wide (1160×770), compact (840×650) and minimum (680×485) editor windows. In the minimum advanced layout the search field occupies its own row. All three filter/import controls have space for their labels in each of the 11 desktop languages. The selection, keyboard focus, long field titles and scrollable notes have dedicated checks in the native suite.
- Captured all four themes from the Windows executable. At 535×440 the picker keeps the primary action visible, including the longer French and Russian captions. The capture process restores the previously selected user preferences.

- The live panel groups resource choices and adjustment buttons. The confirmation view measures the complete value pair using the interface font so the arrow and long signed numbers fit the available space.
- The production plugin built against the local Steam/BepInEx installation. The release passed 73 simulated campaign checks and 206 panel checks, including 24 contrast checks covering grouped controls across the four palettes. The panel screenshots in this release were rendered from the production UI code with sample values.

## Player-requested changes in 0.14.0

- The Windows executable opened the built-in Rizia campaign on a private desktop. Searching for “每回合能源” identified the energy net-change field. At two window sizes, the resource overview displayed current stocks, turn totals and caps. Pending cap adjustments are included in the preview.
- Bookmarked two fields, opened the same campaign in another editor process, filtered the favorites, removed one and checked campaign isolation against the Sordland example. Fifteen isolated registry tests cover persistence, multi-window writes and missing or malformed data. Original game saves were not used for these checks.
- Ran 73 live state checks and 182 panel checks against simulated campaigns and Unity controls. Keyboard and pointer tests exercise all four adjustment buttons, repeated steps, confirmation, in-game value changes, cap changes and integer bounds. The panel tests run in Windows CI at three screen sizes and in four themes. The new relative controls have been exercised in simulation. The earlier live Steam checks are documented below.

## Native editor and files

- Exercised both embedded campaigns through the startup picker. Exported a Sordland sample with its budget changed from 10 to 15 and reopened the output.
- Used multi-word Chinese and raw-key queries on common and advanced pages, including an empty result. Verified selection memory, invalid-input handling, individual reversion, undo/redo and the counts for linked Rizia resource caps.
- Inspected the wide and compact editor layouts in Chinese and English at 200% Windows scaling. The editor screenshots use embedded examples. The save library screenshot uses six synthetic campaign files. Its search, empty results, keyboard focus and campaign choices were exercised in a native Windows dialog.
- Opened a scratch copy of a synthetic save through the executable, applied a budget increase and confirmed that only the expected numeric bytes changed. The pre-save backup matched the original byte for byte. Restoring through the Backups page reproduced the original and kept the modified version as a second backup.
- All 30 synthetic file I/O and setup checks and 159 native editor checks passed, including external modifications, concurrent saves, campaign isolation, the sample overwrite guard, existing plugin handling and theme changes during an active edit.

## Appearance

- Selected all four themes from the actual Windows popup. Exercised both the save picker and editor, including keyboard selection, cancellation, active search, a pending numeric edit, an invalid draft, undo history and restart persistence. Wide and compact layouts were inspected at 200% scaling. The four-theme comparison shows the same embedded Rizia example.
- The native test suite checks localized theme names in all 11 interface languages and exercises missing, invalid and valid preferences in an isolated user registry. The game panel tests independently read and write the same Windows DWORD and reload it when the panel opens.
- In v0.13.2 the native suite checks a minimum text contrast ratio of 4.5:1 for column headings, selected rows, hovered sidebar labels and primary buttons in each theme. The overlay also passed 52 contrast checks across its four palettes.
- Switched all four themes in the running Steam game's Ctrl+F8 panel over the Load Game list. With the panel closed, clicking a campaign group folded it. The same click on an overlaid theme button changed the theme and left the group folded. Closing the panel restored the group's click behavior.

## In-game plugin

- Compiled the .NET 6 IL2CPP plugin against a local Steam installation and BepInEx 6.0.0-be.755 with zero warnings and zero errors. The release includes only the project's plugin DLL.
- All 61 simulated campaign checks and 37 offline panel checks passed for v0.13.2. The panel checks cover keyboard input, confirmation, refresh, theme changes and the pointer target at four screen sizes. The panel image in the guide is an offline rendering with synthetic values.
- Exercised the installer's present/missing-DLL dialogs. From the packaged layout, selected a synthetic game executable through the Windows file chooser and verified the installed plugin bytes.
- In v0.13.0, the plugin ran inside the Windows Steam game with a copy of a real Sordland campaign. The operator changed the government budget from 16 to 17 and restored it to 16. The panel read back both values from the active campaign.
- In a live Sordland campaign, v0.13.1 changed the government budget from 16 to 17 and personal wealth from 1 to 2, and restored both original values. After each of the four writes, the game's native top bar and the Ctrl+F8 panel showed the expected value. The Rizia budget, authority and energy checks used simulated campaigns only.

## Automated and read-only coverage

| Suite | Result |
| --- | --- |
| Parser and renderer | 265 checks passed |
| Session history and patch format | 158 checks passed |
| Save discovery | 23 checks passed. The filesystem symlink test was skipped locally because the account could not create a test symlink. |
| Interface localization | 4,369 checks passed |
| Curated labels | 452 translations across four western languages verified, plus 1,489 checks for eastern languages, Russian and Turkish |
| Display values | 90,256 checks across 11 languages |
| Advanced Chinese names | 24,617 checks across 4,097 fields |
| Local game saves | 233 read-only checks covering 15 Sordland and 6 Rizia files, with zero discovery errors |

For the read-only run, edits and patch round trips were rendered in memory. SHA-256 values before and after matched for every original player save. The published repository includes synthetic fixtures and key/type inventories. Actual player saves are excluded.

The observed save versions were 3.1.0.1.175 and 3.1.0.1.137. Loading numeric changes made with the desktop save editor, in-game event outcomes, unobserved formats and Wine remain unverified. The executable is unsigned.

## Reproduce

Build with `build.ps1 -CompilerKind GCC`, then run `test.ps1 -Compiler <g++.exe>`. Pass `-SaveDirectory <folder>` for the optional read-only check. The default suites use synthetic fixtures. The application's dependency notices are available from Help and in the source tree.

Run `dotnet run --project tests/live/LiveChecks.csproj -c Release` with a .NET 6 SDK to reproduce the simulated campaign suite. To build the actual plugin against your installation, follow the [live panel guide](live/README.zh-CN.md).

The panel test suite can be reproduced on Windows with `dotnet run --project tests/panel/PanelChecks.csproj -c Release`.
