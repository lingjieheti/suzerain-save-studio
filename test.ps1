param(
    [string]$Compiler = "C:\Strawberry\c\bin\g++.exe",
    [string]$SaveDirectory = ""
)
$ErrorActionPreference = "Stop"
$sourceRoot = $PSScriptRoot
$testBuild = Join-Path $sourceRoot "build\tests"
New-Item -ItemType Directory -Force -Path $testBuild | Out-Null
if (!(Test-Path -LiteralPath $Compiler)) { throw "Pass -Compiler with a MinGW x64 g++.exe path." }
$compat = Join-Path $sourceRoot "build\mingw-dwm-compat.hpp"
if (!(Test-Path -LiteralPath $compat)) { throw "Run build.ps1 first (GCC mode)." }
$suites = @("test_core", "test_session", "test_discovery", "test_i18n", "test_field_labels_west", "test_field_labels_east", "test_display_values", "test_advanced_zh", "test_field_favorites", "test_app_io", "test_editor")
if ($SaveDirectory) { $suites += "test_campaign_files" }
foreach ($suite in $suites) {
    $sourceFile = Join-Path $sourceRoot "tests\$suite.cpp"
    if (!(Test-Path -LiteralPath $sourceFile)) { throw "Missing test source: $sourceFile" }
    $binary = Join-Path $testBuild "$suite.exe"
    $flags = @("-std=c++17", "-O2", "-static", "-static-libgcc", "-static-libstdc++", "-include", $compat)
    if ($suite -in @("test_app_io", "test_editor")) {
        # These suites include the application entry point and need its common-controls v6 manifest.
        $flags += Join-Path $sourceRoot "build\app.res"
    }
    $libraries = @("-luser32", "-lkernel32", "-lcomdlg32", "-lcomctl32", "-lshell32", "-lole32", "-luuid", "-lgdi32", "-luxtheme", "-ldwmapi", "-lgdiplus")
    & $Compiler $sourceFile @flags "-o" $binary @libraries
    if ($LASTEXITCODE -ne 0) { throw "$suite compilation failed" }
    $suiteArgs = @()
    if ($suite -in @("test_core", "test_session")) { $suiteArgs = @(Join-Path $sourceRoot "tests\fixture_rizia.json") }
    if ($suite -eq "test_app_io") { $suiteArgs = @(Join-Path $testBuild ("io-" + [guid]::NewGuid().ToString("N"))) }
    if ($suite -eq "test_editor") { $suiteArgs = @((Join-Path $sourceRoot "tests\demo_main.json"), (Join-Path $sourceRoot "tests\demo_rizia.json")) }
    if ($suite -eq "test_campaign_files") { $suiteArgs = @($SaveDirectory) }
    if ($suite -eq "test_display_values") { $suiteArgs = @((Join-Path $sourceRoot "tests\fixture_all_main.json"), (Join-Path $sourceRoot "tests\fixture_all_rizia.json")) }
    if ($suite -eq "test_advanced_zh") { $suiteArgs = @(Join-Path $sourceRoot "tests\advanced_field_keys.txt") }
    & $binary @suiteArgs
    if ($LASTEXITCODE -ne 0) { throw "$suite failed" }
}
Write-Output "All requested suites passed."
