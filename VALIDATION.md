# v0.17.2 Preview — Windows validation

The Windows x64 archive includes the 0.17.2 editor and the optional 0.17.1 in-game plugin. The editor was built with MinGW-w64 GCC 13.2.0 and the plugin with .NET 6 against the local Windows Steam player.

## In-app support in 0.17.2

- The desktop editor passed 860 local checks. All eleven interface languages offered two payment methods, PayPal and USDT on TRON (TRC20). The two retired bitmap resource IDs were absent from the built executable.
- A local QR scan of the documentation found no codes in the ten product screenshots. Each of the two sponsor images decoded to its listed payment destination.

## Ctrl-triggered choice previews in 0.17.1

A standalone left or right Ctrl tap toggles a preview for the pointed choice. Moving to another choice dismisses it. The existing preference is shared between the desktop editor and the game.

- Tested on Windows Steam build 175 with a copy of a Sordland campaign. At the family-origin choices, a left Ctrl tap showed the middle-income effects and a right Ctrl tap dismissed the tooltip on the same option. The dialogue stayed at the selection screen.
- The Windows build passed 98 input/tooltip checks and 307 live panel checks. Regression coverage includes single-frame taps, a key already held when Ctrl is tapped, Ctrl+F8, repeated key events, focus loss, scene changes, and recycled choice controls. The effect parser, dialogue-path and live-state suites passed 47, 54 and 73 checks respectively.

## Choice previews in 0.17.0

- The desktop and in-game switches use the same per-user setting. A new profile starts with previews off. A preview reads the option bound to the hovered control. It does not invoke the option, evaluate Lua or save game state.
- On the Steam Windows 3.1.0 build 175, the release DLL was run with a separate Sordland campaign copy. Hovering the rich, middle-income and poor family origins showed different wealth and public-opinion changes without advancing the dialogue. Wealth still read 0 after comparing all three. Selecting the poor family then took wealth to 1 as previewed. The desktop-enabled switch carried across a game restart and turning it off hid the tooltip. [In-game screenshot](docs/images/choice-preview.jpg)
- The other control families have local binding and synthetic coverage. Their entire story branches have not been played through for this release.
- Synthetic tests exercise Lua and Articy instructions, absolute values and deltas, capped summaries, localized output, hidden single-path dialogue nodes, conditional and cyclic paths. UI tests cover disabled options, binding changes on reused buttons, occluded choices, tooltip geometry, the current carousel item, bill actions and theme changes.
- Local regression runs passed 947 native editor checks, 233 read-only checks across 21 campaign files, 73 simulated live-state checks, 304 panel checks, 47 effect-parser checks, 54 dialogue-path checks and 59 hover checks. The local Windows account skipped symlink creation. The system clipboard test runs on the GitHub-hosted Windows runner.
- To run the standalone choice and pointer suites, use `dotnet run --project tests/choices/ChoiceChecks.csproj -c Release` and `dotnet run --project tests/hover/HoverChecks.csproj -c Release`. Dialogue-path tests run with the existing live-state project.

## In-app support in 0.16.3

- Opened **Support the author** from the save picker and the editor in the extracted release. Closing the window preserved the picker search and an unapplied edit in the Rizia sample. The arrow keys switched methods and Escape returned to the editor.
- Verified the PayPal destination and the TRON address in the app against the published codes.
- Checked the localized donation controls and complete address across all eleven languages. A compact-window layout check exercised 100%, 125%, 150% and 200% scaling. The full local run passed 943 editor checks and 233 read-only campaign checks. Clipboard writes are exercised only on the disposable GitHub-hosted Windows runner. The unchanged optional companion passed 73 simulated state checks and 296 panel checks.

## Chinese font selection in 0.16.2

- Opened both the save picker and editor from the packaged executable on Windows. On this PC, both menus showed **自动（微软雅黑 UI）** checked, offered Noto Sans SC and displayed **苹方 SC（未安装）** disabled. PingFang rendering on a machine with the font installed has not been verified.
- Compared six native views with the same campaign fixtures, window sizes and 200% scaling. Checked the editor and picker at two widths, review and guide search. The final executable reproduced all six candidate screenshots pixel for pixel. [Chinese font comparison](docs/images/chinese-fonts.png)
- The offline PingFang specimen uses only device fonts. Edge checks on this Windows PC covered the missing-font case, disabled JavaScript and viewport widths of 320, 390 and 1024 pixels. The Apple-device sample awaits an on-device visual check.
- The local suite passed 646 native editor checks and 233 read-only campaign checks. It covered retaining draft input and search text while switching fonts. One discovery check for creating a symbolic link was skipped on this local Windows account. The optional companion passed 73 simulated live-state checks and 296 simulated panel checks and matches the shipped 0.16.1 DLL byte for byte.

## Typography in 0.16.1

- Compared the released 0.16.0 editor and the extracted 0.16.1 executable on the same Windows desktop at 200% scaling. Six views used the same synthetic saves and window dimensions. Additional wide and compact Chinese and English views exercised all four themes. The theme/language capture harness isolated user preferences. [Before and after](docs/images/typography.png)
- The native editor suite passed 609 checks. Font measurements cover 100%, 150% and 200% scaling, equal advances for the digits 0–9 and enough vertical space for search and numeric input. At compact width, localized table headings were checked in all eleven languages. A separate two-campaign check measured 396 resource-label extents at three window sizes.
- Installed the packaged companion DLL on the Windows Steam build (3.1.0, build 175). Opened the panel at the main menu with Ctrl+F8 and switched from English to Simplified Chinese. The title, waiting state, buttons, four theme names and shortcut appeared correctly in both languages. All 21 campaign save files retained their original SHA-256 hashes. [In-game screenshot](docs/images/live-panel-preview.png)
- The live suites passed 73 simulated campaign checks and 296 simulated panel checks, including long signed values at compact widths. Live resource writes were tested with the simulated campaigns for this typography update.
- The full local desktop suite passed, including 39 synthetic file-I/O checks and 233 read-only checks on 15 Sordland and 6 Rizia saves. The local discovery suite skipped the symbolic-link creation case because the Windows account could not create the link.

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
