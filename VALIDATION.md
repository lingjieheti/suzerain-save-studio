# v0.11.0 Preview — Windows validation

The release executable was built on Windows with x64 MinGW-w64 GCC 13.2.0. Its version resource reports 0.11.0. The executable imports Windows system and UCRT libraries. No separate application DLL is required.

## Native editor and files

- Exercised both embedded campaigns through the startup picker. Exported a Sordland sample with its budget changed from 10 to 15 and reopened the output.
- Used multi-word Chinese and raw-key queries on common and advanced pages, including an empty result. Verified selection memory, invalid-input handling, individual reversion, undo/redo and the counts for linked Rizia resource caps.
- Inspected wide and compact layouts at 200% Windows scaling. The screenshots use embedded examples.
- Opened a scratch copy of a synthetic save through the executable, applied a budget increase and confirmed that only the expected numeric bytes changed. The pre-save backup matched the original byte for byte. Restoring through the Backups page reproduced the original and kept the modified version as a second backup.
- All 24 synthetic file I/O checks and 41 native editor checks passed, including external modifications, concurrent saves, campaign isolation and the sample overwrite guard.

## Automated and read-only coverage

| Suite | Result |
| --- | --- |
| Parser and renderer | 265 checks passed |
| Session history and patch format | 158 checks passed |
| Save discovery | 23 checks passed. The filesystem symlink test was skipped locally because the account could not create a test symlink. |
| Interface localization | 4,341 checks passed |
| Curated labels | 452 translations across four western languages verified, plus 1,489 checks for eastern languages, Russian and Turkish |
| Display values | 90,256 checks across 11 languages |
| Advanced Chinese names | 24,617 checks across 4,097 fields |
| Local game saves | 233 read-only checks covering 15 Sordland and 6 Rizia files, with zero discovery errors |

For the read-only run, edits and patch round trips were rendered in memory. SHA-256 values before and after matched for every original player save. The published repository includes synthetic fixtures and key/type inventories. Actual player saves are excluded.

The observed game versions were 3.1.0.1.175 and 3.1.0.1.137. Edited saves have not been loaded inside the game. In-game event outcomes, unobserved formats and Wine are unverified. The executable is unsigned.

## Reproduce

Build with `build.ps1 -CompilerKind GCC`, then run `test.ps1 -Compiler <g++.exe>`. Pass `-SaveDirectory <folder>` for the optional read-only check. The default suites use synthetic fixtures. The application's dependency notices are available from Help and in the source tree.
