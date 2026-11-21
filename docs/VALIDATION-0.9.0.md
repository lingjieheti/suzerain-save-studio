# Validation record — Rizia Save Studio 0.9.0

## Release status

**Public preview; native Windows and in-game acceptance are incomplete.** The final GUI candidate has an unresolved intermittent failure in the isolated Wine test environment. This record distinguishes successful component checks from incomplete end-to-end acceptance.

## Build

- C++17, native Win32 Unicode GUI, Windows x64 target.
- Official LLVM-MinGW UCRT toolchain; `-Os -Wall -Wextra -municode -mwindows -static -s`.
- Compilation completed with no warnings.
- PE32+ GUI executable. Imports are Windows system libraries and UCRT API sets; there is no companion application DLL.
- The embedded manifest requests `asInvoker`, not elevation, and declares PerMonitorV2 DPI awareness.
- Runtime notices are embedded as resource 201 and visible in the application's guide page.
- Binary is not code-signed.

## Parser / writer regression

Final core run: **249 checks passed per fixture**.

| Fixture | Bytes | Editable fields | Game version |
|---|---:|---:|---|
| Included synthetic fixture | 3,737 | 44 | 3.1.0.1.175 |
| Private real save, excluded from distribution | 847,619 | 2,617 | 3.1.0.1.175 |

Covered invariants:

- No-op rendering retains every original byte, including formatting and escapes.
- Three resource edits change exactly their three source-value ranges; output parses with identical remaining metadata and field count.
- Numeric and boolean types, scientific notation, UTF-8/BOM, JSON Unicode escapes, Lua comments and nested tables are handled.
- Quoted and nested fake fields are not exposed as top-level variables.
- Truncated or malformed JSON, ambiguous duplicate keys, excessive nesting and oversized files are rejected.
- Unknown keys, type mismatches, nonfinite and out-of-range edits are rejected. Numeric range comparison is exact, avoiding floating-point boundary rounding.
- Saving a non-Rizia story is blocked.

An earlier 248-check test revision also passed AddressSanitizer and UndefinedBehaviorSanitizer. The final test-only revision removes private fixture-specific expectations and adds a generic metadata-preservation assertion. LeakSanitizer was unavailable under this execution environment's ptrace restrictions.

## Session and patch regression

**134 checks passed per fixture**, using both the synthetic and private real save. AddressSanitizer and UndefinedBehaviorSanitizer also passed these checks.

Covered incremental batches, atomic invalid-batch rollback, undo/redo branches, no-op history behavior, clear/reset, exact original-value restoration, unknown keys and type errors, patch round trips, BOM/newline/comment variations, duplicate/injection rejection, and 2,000-entry / 256-KiB limits. Patches contain only keys and target values, without campaign names, paths or save metadata.

## GUI evidence

Execution environment: isolated Wine 9, Xvfb, CJK font substitution. A separate driver used native Win32 controls and real mouse input. Only copies of synthetic fixtures were edited. Byte comparisons were independently performed after save operations.

Successful observations across development candidates:

- Open synthetic save and display resources and metadata.
- Resource 99 preset, whole-batch undo and redo; preview contains six resource/cap values.
- Preview without saving leaves the original byte-identical and creates no backup.
- Save 99 changes exactly six values; backup is byte-identical to the original.
- Budget 250 without manually pressing Apply includes the matching cap; exactly two values change and the prior version is backed up.
- Fractional common-resource input is rejected even where binary floating point could round it to an integer.
- External modification is rejected before overwrite and the externally added bytes remain intact.
- In-app backup selection and two-step restore exactly restore the original, with the pre-restore version separately backed up.
- No temporary write file remained after these completed save/restore operations.

Final candidate visual checks: resources, modification review and 1060×730 compact layout display readable Chinese and accessible primary controls. These screenshots are visual evidence, not proof of a complete successful regression.

### Unresolved GUI test failure

Some ordinary mouse and BM_CLICK sequences produced the Linux native diagnostic `free(): invalid next size (fast)` and invalidated the app window. The location varied, including returning from review or backup pages to resources. The final no-diagnostic-library single-process run encountered this before saving; the test input remained unchanged and no backup was created.

A full sequence passed with a SIGABRT recording library loaded, and shorter normal restore sequences passed. The diagnostic library changes native initialization, memory and timing, so that successful sequence does not establish that the fault is fixed. Static review did not locate a definitive application-side overwrite; root cause and native Windows impact remain unknown.

The final candidate is **not marked as having passed an uninterrupted ordinary GUI regression**. See [known issues](KNOWN-ISSUES-0.9.0.md).

## Remaining boundaries

- No native Windows 10 / 11 desktop acceptance has been executed here.
- No modified save has been loaded or advanced in an actual Suzerain game process.
- Public file-dialog integration for patch import/export and Save As has not completed final end-to-end acceptance; the data/session module has independent test coverage.
- Game events can recalculate income, caps and other values. Numeric relationships do not automatically coordinate story flags or deployed-unit data.
- Advanced variables can have dependencies this editor does not infer.

## Final artifact

SHA-256 and file size are recorded in `SHA256SUMS.txt` and the release notes for the exact distributed EXE. The private real save and internal test logs are not part of the distribution.
